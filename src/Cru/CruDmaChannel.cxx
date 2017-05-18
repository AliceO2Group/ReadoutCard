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

  // Insert links
  getLogger() << "Enabling link(s): ";
  for (uint32_t id : parameters.getLinkMask().value_or(Parameters::LinkMaskType{0})) {
    if (id >= Cru::MAX_LINKS) {
      BOOST_THROW_EXCEPTION(InvalidLinkId()
          << ErrorInfo::Message("CRU does not support given link ID")
          << ErrorInfo::LinkId(id));
    }
    getLogger() << id << " ";
    mLinks.push_back({id});
  }
  getLogger() << InfoLogger::InfoLogger::endm;
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

  if (mFeatures.loopback0x8000020Bar2Register) {
    mPdaBar2.writeRegister(0x8000020 / 4, 1);
  }

  mLinkToPush = 0;
  mLinkToPop = 0;
  mLinksTotalQueueSize = 0;
  for (auto &link : mLinks) {
    link.queue.clear();
    link.superpageCounter = 0;
  }
  mReadyQueue.clear();
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
    getBar().setDataGeneratorPattern(mGeneratorPattern, mGeneratorDataSize);
  }
}

void CruDmaChannel::pushSuperpage(Superpage superpage)
{
  checkSuperpage(superpage);

  if (getTransferQueueAvailable() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
  }

  // Look for an empty slot in the link queues
  for (size_t i = 0; i < mLinks.size(); ++i) {
    auto &link = getNextLinkToPush();
    if (!link.queue.full()) {
      //printf("L %02u - push >>>\n", link.id);
      link.queue.push_back(superpage);
      mLinksTotalQueueSize++;
      auto maxPages = superpage.getSize() / Cru::DMA_PAGE_SIZE;
      auto busAddress = getBusOffsetAddress(superpage.getOffset());
      getBar().pushSuperpageDescriptor(link.id, maxPages, busAddress);
      return;
    }
  }

  // This should never happen
  BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
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
  for (auto& link : mLinks) {
    uint32_t superpageCount = getBar().getSuperpageCount(link.id);
//    printf("L %02u - superpage_count=%u\n", link.id, superpageCount);
    auto available = superpageCount > link.superpageCounter;
    if (available) {
      uint32_t amountAvailable = superpageCount - link.superpageCounter;
      for (uint32_t i = 0; i < amountAvailable; ++i) {
        if (mReadyQueue.full()) {
          break;
        }
        //printf("L %02u - pop <<<\n", link.id);

        // Front superpage has arrived
        link.queue.front().ready = true;
        link.queue.front().received = link.queue.front().size;
        mReadyQueue.push_back(link.queue.front());
        link.queue.pop_front();
        link.superpageCounter++;
        mLinksTotalQueueSize--;
      }
    }
  }
}

int CruDmaChannel::getTransferQueueAvailable()
{
  // capacity - size = available
  return (mLinks.size() * LINK_QUEUE_CAPACITY) - mLinksTotalQueueSize;
}

int CruDmaChannel::getReadyQueueSize()
{
  return mReadyQueue.size();
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
  return boost::str(boost::format("%x-%x-%x") % getBar2().getFirmwareDate() % getBar2().getFirmwareTime()
      % getBar2().getFirmwareGitHash());
}

} // namespace roc
} // namespace AliceO2
