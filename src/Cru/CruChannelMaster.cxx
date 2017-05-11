/// \file CruChannelMaster.cxx
/// \brief Implementation of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruChannelMaster.h"
#include <boost/dynamic_bitset.hpp>
#include <thread>
#include <sstream>
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
namespace Rorc
{
namespace
{
/// DMA page length in bytes
/// Note: the CRU has a firmware defined fixed page size
constexpr int DMA_PAGE_SIZE = 8 * 1024;

/// DMA addresses must be 32-byte aligned
constexpr uint64_t DMA_ALIGNMENT = 32;
} // Anonymous namespace

CruChannelMaster::CruChannelMaster(const Parameters& parameters)
    : ChannelMasterPdaBase(parameters, allowedChannels()), //
      mInitialResetLevel(ResetLevel::Rorc), // It's good to reset at least the card channel in general
      mLoopbackMode(parameters.getGeneratorLoopback().get_value_or(LoopbackMode::Rorc)), // Internal loopback by default
      mGeneratorEnabled(parameters.getGeneratorEnabled().get_value_or(true)), // Use data generator by default
      mGeneratorPattern(parameters.getGeneratorPattern().get_value_or(GeneratorPattern::Incremental)), //
      mGeneratorMaximumEvents(0), // Infinite events
      mGeneratorInitialValue(0), // Start from 0
      mGeneratorInitialWord(0), // First word
      mGeneratorSeed(0), // Presumably for random patterns, incremental doesn't really need it
      mGeneratorDataSize(parameters.getGeneratorDataSize().get_value_or(DMA_PAGE_SIZE)) // Can use page size
{
  if (auto pageSize = parameters.getDmaPageSize()) {
    if (pageSize.get() != DMA_PAGE_SIZE) {
      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("CRU only supports an 8kB page size")
          << ErrorInfo::DmaPageSize(pageSize.get()));
    }
  }

  if (auto enabled = parameters.getGeneratorEnabled()) {
    if (enabled.get() == false) {
      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("CRU currently only supports operation with data generator"));
    }
  }

  Utilities::resetSmartPtr(mPdaBar2, getRorcDevice().getPciDevice(), 2);

  // Insert links
  getLogger() << "Enabling link ";
  for (auto id : parameters.getLinkMask().value_or(Parameters::LinkMaskType{0})) {
    getLogger() << id << " ";
    mLinks.push_back({id});
  }
  getLogger() << InfoLogger::InfoLogger::endm;
}

auto CruChannelMaster::allowedChannels() -> AllowedChannels {
  // Note: BAR 1 is not available because BAR 0 is 64-bit wide, so it 'consumes' two BARs.
  return {0};
}

CruChannelMaster::~CruChannelMaster()
{
  setBufferNonReady();
}

void CruChannelMaster::deviceStartDma()
{
  resetCru();
  initCru();
  setBufferReady();

  /// XXX TEMPORARY SETTING FOR A SPECIFIC DATA GENERATOR
  mPdaBar2->writeRegister(0x8000020/4, 1);

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
void CruChannelMaster::setBufferReady()
{
  getBar().setDataEmulatorEnabled(true);
  std::this_thread::sleep_for(10ms);
}

/// Set buffer to non-ready
void CruChannelMaster::setBufferNonReady()
{
  getBar().setDataEmulatorEnabled(false);
}

void CruChannelMaster::deviceStopDma()
{
  setBufferNonReady();
}

void CruChannelMaster::deviceResetChannel(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::Nothing) {
    return;
  }

  resetCru();
}

CardType::type CruChannelMaster::getCardType()
{
  return CardType::Cru;
}

void CruChannelMaster::resetCru()
{
  getBar().resetDataGeneratorCounter();
  std::this_thread::sleep_for(100ms);
  getBar().resetCard();
  std::this_thread::sleep_for(100ms);
}

void CruChannelMaster::initCru()
{
  // Set data generator pattern
  if (mGeneratorEnabled) {
    getBar().setDataGeneratorPattern(mGeneratorPattern);
  }
}

void CruChannelMaster::pushSuperpage(Superpage superpage)
{
  checkSuperpage(superpage);

  if (getTransferQueueAvailable() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
  }

    // Look for an empty slot in the link queues
  for (size_t i = 0; i < mLinks.size(); ++i) {
    auto &link = getNextLinkToPush();
    if (!link.queue.full()) {
      link.queue.push_back(superpage);
      mLinksTotalQueueSize++;
      auto maxPages = superpage.getSize() / DMA_PAGE_SIZE;
      auto busAddress = getBusOffsetAddress(superpage.getOffset());
      getBar().pushSuperpageDescriptor(maxPages, busAddress);
      return;
    }
  }

  // This should never happen
  BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
}

auto CruChannelMaster::getSuperpage() -> Superpage
{
  if (mReadyQueue.empty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get superpage, ready queue was empty"));
  }

  return mReadyQueue.front();
}

auto CruChannelMaster::popSuperpage() -> Superpage
{
  if (mReadyQueue.empty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, ready Queue was empty"));
  }

  auto superpage = mReadyQueue.front();
  mReadyQueue.pop_front();
  return superpage;
}

void CruChannelMaster::fillSuperpages()
{
  // Check for arrivals & handle them
  for (auto& link : mLinks) {
    uint32_t superpageCount = getBar().getSuperpageCount();
    auto available = superpageCount > link.superpageCounter;
    if (available) {
      for (uint32_t i = 0; i < (superpageCount - link.superpageCounter); ++i) {
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

int CruChannelMaster::getTransferQueueAvailable()
{
  // capacity - size = available
  return (mLinks.size() * LINK_QUEUE_CAPACITY) - mLinksTotalQueueSize;
}

int CruChannelMaster::getReadyQueueSize()
{
  return mReadyQueue.size();
}

boost::optional<float> CruChannelMaster::getTemperature()
{
  return getBar2().getTemperatureCelsius();
}

boost::optional<std::string> CruChannelMaster::getFirmwareInfo()
{
  std::ostringstream stream;
  stream << getBar2().getFirmwareDate() << '-' << getBar2().getFirmwareTime() << '-' << std::hex
       << getBar2().getFirmwareGitHash();
  return stream.str();
}

} // namespace Rorc
} // namespace AliceO2
