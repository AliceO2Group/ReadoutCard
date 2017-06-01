/// \file CruDmaChannel.cxx
/// \brief Implementation of the CruDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruDmaChannel.h"
#include <sstream>
#include <thread>
#include <boost/format.hpp>
#include "ChannelPaths.h"
#include "ExceptionInternal.h"
#include "Utilities/SmartPointer.h"

/// Creates a CruException and attaches data using the given message string
#define CRU_EXCEPTION(_err_message) \
  CruException() \
      << errinfo_rorc_generic_message(_err_message)

using namespace std::literals;

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
      mLoopbackMode(parameters.getGeneratorLoopback().get_value_or(LoopbackMode::Internal)), // Internal loopback by default
      mGeneratorEnabled(parameters.getGeneratorEnabled().get_value_or(true)), // Use data generator by default
      mGeneratorPattern(parameters.getGeneratorPattern().get_value_or(GeneratorPattern::Incremental)), //
      mGeneratorDataSizeRandomEnabled(parameters.getGeneratorRandomSizeEnabled().get_value_or(true)), //
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

  if (auto enabled = parameters.getGeneratorEnabled()) {
    if (enabled.get() == false) {
      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("CRU currently only supports operation with data generator"));
    }
  }

  if (mFeatures.standalone) {
    auto logFeature = [&](auto name, bool enabled) { if (!enabled) { getLogger() << " " << name; }};
    getLogger() << "Standalone firmware features disabled:";
    logFeature("firmware-info", mFeatures.firmwareInfo);
    logFeature("serial-number", mFeatures.serial);
    logFeature("temperature", mFeatures.temperature);
    logFeature("data-selection", mFeatures.dataSelection);
    getLogger() << InfoLogger::InfoLogger::endm;
  }

  // Insert links
  {
    getLogger() << "Enabling link(s): ";
    auto linkMask = parameters.getLinkMask().value_or(Parameters::LinkMaskType{0});
    mLinks.reserve(linkMask.size());
    for (uint32_t id : linkMask) {
      if (id >= Cru::MAX_LINKS) {
        BOOST_THROW_EXCEPTION(InvalidLinkId() << ErrorInfo::Message("CRU does not support given link ID")
          << ErrorInfo::LinkId(id));
      }
      getLogger() << id << " ";
      mLinks.push_back({static_cast<LinkId>(id)});
    }
    getLogger() << InfoLogger::InfoLogger::endm;
  }
}

auto CruDmaChannel::allowedChannels() -> AllowedChannels {
  // We have only one DMA channel per CRU
  return {0};
}

CruDmaChannel::~CruDmaChannel()
{
  setBufferNonReady();
}

void CruDmaChannel::deviceStartDma()
{
  resetCru();
  initCru();
  setBufferReady();

  if (mFeatures.dataSelection) {
    // Something with selecting the data source... [insert link to documentation here]
    mPdaBar2.writeRegister(0x8000020 / 4, 1);
  }

  for (auto &link : mLinks) {
    link.queue.clear();
    link.superpageCounter = 0;
  }
  mReadyQueue.clear();

  mLinkQueuesTotalAvailable = LINK_QUEUE_CAPACITY * mLinks.size();
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

void CruDmaChannel::initCru()
{
  // Set data generator pattern
  if (mGeneratorEnabled) {
    getBar().setDataGeneratorPattern(mGeneratorPattern, mGeneratorDataSize, mGeneratorDataSizeRandomEnabled);
  }
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
//  printf("L %02u push - tav=%lu\n", link.id, mLinkQueuesTotalAvailable);
  mLinkQueuesTotalAvailable--;
  link.queue.push_back(superpage);

//  printf("                        SIZES:");
//  for (LinkIndex linkIndex = 0; linkIndex < mLinks.size(); ++linkIndex) {
//    printf(" %lu", mLinks[linkIndex].queue.size());
//  }
//  printf("\n");

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
        getLogger() << InfoLogger::InfoLogger::Error
          << "FATAL: Firmware reported more superpages available (" << amountAvailable <<
          ") than should be present in FIFO (" << link.queue.size() << "); "
          << link.superpageCounter << " superpages received from link " << int(link.id) << " according to driver, "
          << superpageCount << " pushed according to firmware" << InfoLogger::InfoLogger::endm;
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("FATAL: Firmware reported more superpages available than should be present in FIFO"));
      }

      for (uint32_t i = 0; i < amountAvailable; ++i) {
        if (mReadyQueue.size() >= READY_QUEUE_CAPACITY) {
          break;
        }

        // Front superpage has arrived
        link.queue.front().ready = true;
        link.queue.front().received = link.queue.front().size;
        mReadyQueue.push_back(link.queue.front());
        mLinkQueuesTotalAvailable++;
        link.queue.pop_front();
        link.superpageCounter++;

//        printf("  L %02u pop - tav=%lu qsize=%lu spcount=%u \n", link.id, mLinkQueuesTotalAvailable,
//          link.queue.size(), link.superpageCounter);
//
//        printf("                        SIZES:");
//        for (LinkIndex linkIndex = 0; linkIndex < mLinks.size(); ++linkIndex) {
//          printf(" %lu", mLinks[linkIndex].queue.size());
//        }
//        printf("\n");
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
    return boost::str(boost::format("%x-%x-%x") % getBar2().getFirmwareDate() % getBar2().getFirmwareTime()
        % getBar2().getFirmwareGitHash());
  } else {
    return {};
  }
}

} // namespace roc
} // namespace AliceO2
