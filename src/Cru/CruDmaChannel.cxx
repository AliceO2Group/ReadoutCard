/// \file CruDmaChannel.cxx
/// \brief Implementation of the CruDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruDmaChannel.h"
#include <thread>
#include <boost/format.hpp>
#include "ExceptionInternal.h"

using namespace std::literals;
using boost::format;

namespace AliceO2
{
namespace roc
{

CruDmaChannel::CruDmaChannel(const Parameters& parameters)
    : DmaChannelPdaBase(parameters, allowedChannels()), //
      mPdaBar(getRocPciDevice().getPciDevice(), 0), // Initialize BAR 0
      mPdaBar2(getRocPciDevice().getPciDevice(), 2), // Initialize BAR 2
      mFeatures(getBar().getFirmwareFeatures()), // Get which features of the firmware are enabled
      mInitialResetLevel(ResetLevel::Internal), // It's good to reset at least the card channel in general
      mLoopbackMode(parameters.getGeneratorLoopback().get_value_or(LoopbackMode::None)), // No loopback by default
      mGeneratorEnabled(parameters.getGeneratorEnabled().get_value_or(true)), // Use data generator by default
      mGeneratorPattern(parameters.getGeneratorPattern().get_value_or(GeneratorPattern::Incremental)), //
      mGeneratorDataSizeRandomEnabled(parameters.getGeneratorRandomSizeEnabled().get_value_or(false)), //
      mGeneratorMaximumEvents(0), // Infinite events
      mGeneratorInitialValue(0), // Start from 0
      mGeneratorInitialWord(0), // First word
      mGeneratorSeed(0), // Presumably for random patterns, incremental doesn't really need it
      mGeneratorDataSize(parameters.getGeneratorDataSize().get_value_or(Cru::DMA_PAGE_SIZE)) // Can use page size
{
  if (auto pageSize = parameters.getDmaPageSize()) {
    if (pageSize.get() != Cru::DMA_PAGE_SIZE) {
      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("CRU only supports an 8KiB page size")
          << ErrorInfo::DmaPageSize(pageSize.get()));
    }
  }

  if (mLoopbackMode == LoopbackMode::Diu || mLoopbackMode == LoopbackMode::Siu) {
    BOOST_THROW_EXCEPTION(CruException() << ErrorInfo::Message("CRU does not support given loopback mode")
      << ErrorInfo::LoopbackMode(mLoopbackMode));
  }

  if (mFeatures.standalone) {
    std::stringstream stream;
    auto logFeature = [&](auto name, bool enabled) { if (!enabled) { stream << " " << name; }};
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
    auto linkMask = parameters.getLinkMask().value_or(Parameters::LinkMaskType{0});
    mLinks.reserve(linkMask.size());
    for (uint32_t id : linkMask) {
      if (id >= Cru::MAX_LINKS) {
        BOOST_THROW_EXCEPTION(InvalidLinkId() << ErrorInfo::Message("CRU does not support given link ID")
          << ErrorInfo::LinkId(id));
      }
      stream << id << " ";
      mLinks.push_back({static_cast<LinkId>(id)});
    }
    log(stream.str());
  }
  
  // If the Generator is enabled and no LoopbackMode was specified, fallback to Internal
  if (mGeneratorEnabled && mLoopbackMode == LoopbackMode::None) {
    log("No loopback mode specified; defaulting to 'Internal'", InfoLogger::InfoLogger::Info);
    mLoopbackMode = LoopbackMode::Internal;
  }
}

auto CruDmaChannel::allowedChannels() -> AllowedChannels {
  // We have only one DMA channel per CRU endpoint
  return {0};
}

CruDmaChannel::~CruDmaChannel()
{
  setBufferNonReady();
  if (mReadyQueue.size() > 0) {
    log((format("Remaining superpages in the ready queue: %1%") % mReadyQueue.size()).str());
  }
}

void CruDmaChannel::deviceStartDma()
{
  // Enable links
  uint32_t mask = 0xFfffFfff;
  for (const auto& link : mLinks) {
    Utilities::setBit(mask, link.id, false);
  }
  getBar().setLinksEnabled(mask);

  // Set data generator pattern
  if (mGeneratorEnabled) {
    getBar().setDataGeneratorPattern(mGeneratorPattern, mGeneratorDataSize, mGeneratorDataSizeRandomEnabled);
  }

  // Set data source
  if (mGeneratorEnabled) {
    if (mLoopbackMode == LoopbackMode::Internal) {
      if (mFeatures.dataSelection) {
        getBar().setDataSource(Cru::Registers::DATA_SOURCE_SELECT_INTERNAL);
      } else {
        log("Did not set internal data source, feature not supported by firmware", InfoLogger::InfoLogger::Warning);
      }
    } else {
      BOOST_THROW_EXCEPTION(CruException()
        << ErrorInfo::Message("CRU only supports 'Internal' loopback for data generator"));
    }
  } else {
    if (mLoopbackMode == LoopbackMode::None) {
      if (mFeatures.dataSelection) {
        getBar().setDataSource(Cru::Registers::DATA_SOURCE_SELECT_GBT);
      } else {
        log("Did not set GBT data source, feature not supported by firmware", InfoLogger::InfoLogger::Warning);
      }
    } else {
      BOOST_THROW_EXCEPTION(CruException()
        << ErrorInfo::Message("CRU only supports 'None' loopback when operating without data generator"));
    }
  }

  // Reset CRU (should be done after link mask set)
  resetCru();

  // Initialize link queues
  for (auto &link : mLinks) {
    link.queue.clear();
    link.superpageCounter = 0;
  }
  mReadyQueue.clear();
  mLinkQueuesTotalAvailable = LINK_QUEUE_CAPACITY * mLinks.size();

  // Start DMA
  setBufferReady();
}

/// Set buffer to ready
void CruDmaChannel::setBufferReady()
{
  getBar().setDataEmulatorEnabled(true);
  std::this_thread::sleep_for(10ms);
}

/// Set buffer to non-ready
void CruDmaChannel::setBufferNonReady()
{
  getBar().setDataEmulatorEnabled(false);
}

void CruDmaChannel::deviceStopDma()
{
  setBufferNonReady();
  int moved = 0;
  for (auto& link : mLinks) {
    size_t size = link.queue.size();
    for (size_t i = 0; i < size; ++i) {
      transferSuperpageFromLinkToReady(link);
      moved++;
    }
    assert(link.queue.empty());
  }
  assert(mLinkQueuesTotalAvailable == LINK_QUEUE_CAPACITY * mLinks.size());
  log((format("Moved %1% remaining superpages to ready queue") % moved).str());
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
  getBar().resetDataGeneratorCounter();
  std::this_thread::sleep_for(100ms);
  getBar().resetCard();
  std::this_thread::sleep_for(100ms);
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
  auto &link = mLinks[getNextLinkIndex()];

  if (link.queue.size() >= LINK_QUEUE_CAPACITY) {
    // Is the link's FIFO out of space?
    // This should never happen
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, link queue was full"));
  }

  // Once we've confirmed the link has a slot available, we push the superpage
  pushSuperpageToLink(link, superpage);
  auto maxPages = superpage.getSize() / Cru::DMA_PAGE_SIZE;
  auto busAddress = getBusOffsetAddress(superpage.getOffset());
  getBar().pushSuperpageDescriptor(link.id, maxPages, busAddress);
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
  link.queue.front().ready = true;
  link.queue.front().received = link.queue.front().size;
  mReadyQueue.push_back(link.queue.front());
  mLinkQueuesTotalAvailable++;
  link.queue.pop_front();
  link.superpageCounter++;
}

void CruDmaChannel::fillSuperpages()
{
  // Check for arrivals & handle them
  const auto size = mLinks.size();
  for (LinkIndex linkIndex = 0; linkIndex < size; ++linkIndex) {
    auto& link = mLinks[linkIndex];
    uint32_t superpageCount = getBar().getSuperpageCount(link.id);
    auto available = superpageCount > link.superpageCounter;
    if (available) {
      uint32_t amountAvailable = superpageCount - link.superpageCounter;
      if (amountAvailable > link.queue.size()) {
        std::stringstream stream;
        stream << "FATAL: Firmware reported more superpages available (" << amountAvailable <<
          ") than should be present in FIFO (" << link.queue.size() << "); "
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

int CruDmaChannel::getReadyQueueSize()
{
  return mReadyQueue.size();
}

bool CruDmaChannel::injectError()
{
  if (mGeneratorEnabled) {
    getBar().dataGeneratorInjectError();
    return true;
  } else {
    return false;
  }
}

boost::optional<int32_t> CruDmaChannel::getSerial()
{
  if (mFeatures.serial) {
    return getBar2().getSerialNumber();
  } else {
    return {};
  }
}

boost::optional<float> CruDmaChannel::getTemperature()
{
  if (mFeatures.temperature) {
    return getBar2().getTemperatureCelsius();
  } else {
    return {};
  }
}

boost::optional<std::string> CruDmaChannel::getFirmwareInfo()
{
  if (mFeatures.firmwareInfo) {
    return (boost::format("%x-%x-%x") % getBar2().getFirmwareDate() % getBar2().getFirmwareTime()
        % getBar2().getFirmwareGitHash()).str();
  } else {
    return {};
  }
}

boost::optional<std::string> CruDmaChannel::getCardId()
{
  if (mFeatures.chipId) {
    return (boost::format("%08x-%08x") % getBar2().getFpgaChipId1() % getBar2().getFpgaChipId2()).str();
  } else {
    return {};
  }
}


} // namespace roc
} // namespace AliceO2
