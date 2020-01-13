// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CruDmaChannel.cxx
/// \brief Implementation of the CruDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <thread>
#include <boost/format.hpp>
#include "CruDmaChannel.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/ChannelFactory.h"

using namespace std::literals;
using boost::format;

namespace AliceO2
{
namespace roc
{

CruDmaChannel::CruDmaChannel(const Parameters& parameters)
  : DmaChannelPdaBase(parameters, allowedChannels()),                           //
    mInitialResetLevel(ResetLevel::Internal),                                   // It's good to reset at least the card channel in general
    mDataSource(parameters.getDataSource().get_value_or(DataSource::Internal)), // DG loopback mode by default
    mDmaPageSize(parameters.getDmaPageSize().get_value_or(Cru::DMA_PAGE_SIZE))
{

  if (auto pageSize = parameters.getDmaPageSize()) {
    if (pageSize.get() != Cru::DMA_PAGE_SIZE) {
      log("DMA page size not default; Behaviour undefined", InfoLogger::InfoLogger::Warning);
      /*BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("CRU only supports an 8KiB page size")
          << ErrorInfo::DmaPageSize(pageSize.get()));*/
    }
  }

  if (mDataSource == DataSource::Diu || mDataSource == DataSource::Siu) {
    BOOST_THROW_EXCEPTION(CruException() << ErrorInfo::Message("CRU does not support specified data source")
                                         << ErrorInfo::DataSource(mDataSource));
  }

  // Prep for BARs
  auto parameters2 = parameters;
  parameters2.setChannelNumber(2);
  auto bar = ChannelFactory().getBar(parameters);
  auto bar2 = ChannelFactory().getBar(parameters2);
  cruBar = std::move(std::dynamic_pointer_cast<CruBar>(bar));   // Initialize BAR 0
  cruBar2 = std::move(std::dynamic_pointer_cast<CruBar>(bar2)); // Initialize BAR 2
  mFeatures = getBar()->getFirmwareFeatures();                  // Get which features of the firmware are enabled

  if (mFeatures.standalone) {
    std::stringstream stream;
    auto logFeature = [&](auto name, bool enabled) { if (!enabled) { stream << " " << name; } };
    stream << "Standalone firmware features disabled:";
    logFeature("firmware-info", mFeatures.firmwareInfo);
    logFeature("serial-number", mFeatures.serial);
    logFeature("temperature", mFeatures.temperature);
    logFeature("data-selection", mFeatures.dataSelection);
    log(stream.str());
  }

  // Insert links
  {
    std::stringstream stream;
    stream << "Enabling link(s): ";
    auto linkMask = parameters.getLinkMask().value_or(Parameters::LinkMaskType{ 0 });
    mLinks.reserve(linkMask.size());
    for (uint32_t id : linkMask) {
      if (id >= Cru::MAX_LINKS) {
        BOOST_THROW_EXCEPTION(InvalidLinkId() << ErrorInfo::Message("CRU does not support given link ID")
                                              << ErrorInfo::LinkId(id));
      }
      stream << id << " ";
      mLinks.push_back({ static_cast<LinkId>(id) });
    }
    log(stream.str());
  }
}

auto CruDmaChannel::allowedChannels() -> AllowedChannels
{
  // We have only one DMA channel per CRU endpoint
  return { 0 };
}

CruDmaChannel::~CruDmaChannel()
{
  setBufferNonReady();
  if (mReadyQueue.size() > 0) {
    log((format("Remaining superpages in the ready queue: %1%") % mReadyQueue.size()).str());
  }

  if (mDataSource == DataSource::Internal) {
    resetDebugMode();
  }
}

void CruDmaChannel::deviceStartDma()
{
  // Set data source
  uint32_t dataSourceSelection = 0x0;

  if (mDataSource == DataSource::Internal) {
    enableDebugMode();
    dataSourceSelection = Cru::Registers::DATA_SOURCE_SELECT_INTERNAL;
  } else { // data source == FEE or DDG
    dataSourceSelection = Cru::Registers::DATA_SOURCE_SELECT_GBT;
  }

  if (mFeatures.dataSelection) {
    getBar()->setDataSource(dataSourceSelection);
  } else {
    log("Did not set data source, feature not supported by firmware", InfoLogger::InfoLogger::Warning);
  }

  // Reset CRU (should be done after link mask set)
  resetCru();

  // Initialize link queues
  for (auto& link : mLinks) {
    link.queue.clear();
    link.superpageCounter = 0;
  }
  mReadyQueue.clear();
  mLinkQueuesTotalAvailable = LINK_QUEUE_CAPACITY * mLinks.size();

  // Start DMA
  setBufferReady();

  // Enable data taking
  if (dataSourceSelection == Cru::Registers::DATA_SOURCE_SELECT_GBT) {
    getBar2()->disableDataTaking(); // Make sure we don't start from a bad state
    getBar2()->enableDataTaking();
  }
}

/// Set buffer to ready
void CruDmaChannel::setBufferReady()
{
  getBar()->startDmaEngine();
  std::this_thread::sleep_for(10ms);
}

/// Set buffer to non-ready
void CruDmaChannel::setBufferNonReady()
{
  getBar()->stopDmaEngine();
}

void CruDmaChannel::deviceStopDma()
{
  // Disable data taking
  setBufferNonReady();
  getBar2()->disableDataTaking();

  // Transfer remaining (filled) superpages to ReadyQueue
  int moved = 0;
  for (auto& link : mLinks) {
    int32_t superpageCount = getBar()->getSuperpageCount(link.id);
    uint32_t amountAvailable = superpageCount - link.superpageCounter;
    //log((format("superpageCount %1% amountAvailable %2%") % superpageCount % amountAvailable).str());
    while (amountAvailable) {
      if (mReadyQueue.size() >= READY_QUEUE_CAPACITY) {
        break;
      }

      if (!link.queue.empty()) {
        transferSuperpageFromLinkToReady(link);
        moved++;
      }
      amountAvailable--;
    }
  }

  // Return any superpages that have been pushed up in the meantime but won't get filled
  for (auto& link : mLinks) {
    while (!link.queue.empty()) {
      link.queue.front().setReceived(0);
      link.queue.front().setReady(false);
      mReadyQueue.push_back(link.queue.front());
      link.queue.pop_front();
    }

    if (!link.queue.empty()) {
      log((format("Superpage queue of link %1% not empty after DMA stop. Superpages unclaimed.") % link.id).str(),
          InfoLogger::InfoLogger::Error);
    }
  }

  if (getTransferQueueAvailable()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Transfer queue not full when it should be"));
  }

  log((format("Moved %1% remaining superpage(s) to ready queue") % moved).str());
}

void CruDmaChannel::deviceResetChannel(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::Nothing) {
    return;
  }
  resetCru();
}

CardType::type CruDmaChannel::getCardType()
{
  return CardType::Cru;
}

void CruDmaChannel::resetCru()
{
  getBar()->resetDataGeneratorCounter();
  std::this_thread::sleep_for(100ms);
  getBar()->resetCard();
  std::this_thread::sleep_for(100ms);
  getBar()->resetInternalCounters();
}

auto CruDmaChannel::getNextLinkIndex() -> LinkIndex
{
  auto smallestQueueIndex = std::numeric_limits<LinkIndex>::max();
  auto smallestQueueSize = std::numeric_limits<size_t>::max();

  for (size_t i = 0; i < mLinks.size(); ++i) {
    auto queueSize = mLinks[i].queue.size();
    if (queueSize < smallestQueueSize) {
      smallestQueueIndex = i;
      smallestQueueSize = queueSize;
    }
  }

  return smallestQueueIndex;
}

void CruDmaChannel::pushSuperpage(Superpage superpage)
{
  checkSuperpage(superpage);

  if (mLinkQueuesTotalAvailable == 0) {
    // Note: the transfer queue refers to the firmware, not the mLinkIndexQueue which contains the LinkIds for links
    // that can still be pushed into (essentially the opposite of the firmware's queue).
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
  }

  // Get the next link to push
  auto& link = mLinks[getNextLinkIndex()];

  if (link.queue.size() >= LINK_QUEUE_CAPACITY) {
    // Is the link's FIFO out of space?
    // This should never happen
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, link queue was full"));
  }

  // Once we've confirmed the link has a slot available, we push the superpage
  pushSuperpageToLink(link, superpage);
  auto dmaPages = superpage.getSize() / mDmaPageSize;
  auto busAddress = getBusOffsetAddress(superpage.getOffset());
  getBar()->pushSuperpageDescriptor(link.id, dmaPages, busAddress);
}

auto CruDmaChannel::getSuperpage() -> Superpage
{
  if (mReadyQueue.empty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get superpage, ready queue was empty"));
  }
  return mReadyQueue.front();
}

auto CruDmaChannel::popSuperpage() -> Superpage
{
  if (mReadyQueue.empty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, ready queue was empty"));
  }
  auto superpage = mReadyQueue.front();
  mReadyQueue.pop_front();
  return superpage;
}

void CruDmaChannel::pushSuperpageToLink(Link& link, const Superpage& superpage)
{
  mLinkQueuesTotalAvailable--;
  link.queue.push_back(superpage);
}

void CruDmaChannel::transferSuperpageFromLinkToReady(Link& link)
{
  if (link.queue.empty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not transfer Superpage from link to ready queue, link queue is empty"));
  }

  link.queue.front().setReady(true);

  uint32_t superpageSize = getBar()->getSuperpageSize(link.id);
  if (superpageSize == 0) {
    link.queue.front().setReceived(link.queue.front().getSize()); // force the full superpage size for backwards compatibility
  } else {
    link.queue.front().setReceived(superpageSize);
  }

  mReadyQueue.push_back(link.queue.front());
  link.queue.pop_front();
  link.superpageCounter++;
  mLinkQueuesTotalAvailable++;
}

void CruDmaChannel::fillSuperpages()
{
  // Check for arrivals & handle them
  const auto links = mLinks.size();
  for (LinkIndex linkIndex = 0; linkIndex < links; ++linkIndex) {
    auto& link = mLinks[linkIndex];
    uint32_t superpageCount = getBar()->getSuperpageCount(link.id);
    auto available = superpageCount > link.superpageCounter;
    if (available) {
      uint32_t amountAvailable = superpageCount - link.superpageCounter;
      if (amountAvailable > link.queue.size()) {
        std::stringstream stream;
        stream << "FATAL: Firmware reported more superpages available (" << amountAvailable << ") than should be present in FIFO (" << link.queue.size() << "); "
               << link.superpageCounter << " superpages received from link " << int(link.id) << " according to driver, "
               << superpageCount << " pushed according to firmware";
        log(stream.str(), InfoLogger::InfoLogger::Error);
        BOOST_THROW_EXCEPTION(Exception()
                              << ErrorInfo::Message("FATAL: Firmware reported more superpages available than should be present in FIFO"));
      }

      for (uint32_t i = 0; i < amountAvailable; ++i) {
        if (mReadyQueue.size() >= READY_QUEUE_CAPACITY) {
          break;
        }
        // Front superpage has arrived
        transferSuperpageFromLinkToReady(link);
      }
    }
  }
}

int CruDmaChannel::getTransferQueueAvailable()
{
  return mLinkQueuesTotalAvailable;
}

// Return a boolean that denotes whether the transfer queue is empty
// The transfer queue is empty when all its slots are available
bool CruDmaChannel::isTransferQueueEmpty()
{
  return mLinkQueuesTotalAvailable == (LINK_QUEUE_CAPACITY * mLinks.size());
}

int CruDmaChannel::getReadyQueueSize()
{
  return mReadyQueue.size();
}

// Return a boolean that denotes whether the ready queue is full
// The ready queue is full when the CRU has filled it up
bool CruDmaChannel::isReadyQueueFull()
{
  return mReadyQueue.size() == READY_QUEUE_CAPACITY;
}

int32_t CruDmaChannel::getDroppedPackets()
{
  int endpoint = getBar()->getEndpointNumber();
  return getBar2()->getDroppedPackets(endpoint);
}

bool CruDmaChannel::injectError()
{
  if (!mDataSource == DataSource::Fee) {
    getBar()->dataGeneratorInjectError();
    return true;
  } else {
    return false;
  }
}

void CruDmaChannel::enableDebugMode()
{
  if (!getBar()->getDebugModeEnabled()) {
    getBar()->setDebugModeEnabled(true);
    mDebugRegisterReset = true;
  }
}

void CruDmaChannel::resetDebugMode()
{
  if (mDebugRegisterReset) {
    getBar()->setDebugModeEnabled(false);
  }
}

boost::optional<int32_t> CruDmaChannel::getSerial()
{
  if (mFeatures.serial) {
    return getBar2()->getSerial();
  } else {
    return {};
  }
}

boost::optional<float> CruDmaChannel::getTemperature()
{
  if (mFeatures.temperature) {
    return getBar2()->getTemperature();
  } else {
    return {};
  }
}

boost::optional<std::string> CruDmaChannel::getFirmwareInfo()
{
  if (mFeatures.firmwareInfo) {
    return getBar2()->getFirmwareInfo();
  } else {
    return {};
  }
}

boost::optional<std::string> CruDmaChannel::getCardId()
{
  if (mFeatures.chipId) {
    return getBar2()->getCardId();
  } else {
    return {};
  }
}

} // namespace roc
} // namespace AliceO2
