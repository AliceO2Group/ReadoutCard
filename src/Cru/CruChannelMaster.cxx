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

/// DMA page length in 32-bit words
constexpr int DMA_PAGE_SIZE_32 = DMA_PAGE_SIZE / 4;

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
}

auto CruChannelMaster::allowedChannels() -> AllowedChannels {
  // Note: BAR 1 is not available because BAR 0 is 64-bit wide, so it 'consumes' two BARs.
  return {0, 2};
}

CruChannelMaster::~CruChannelMaster()
{
  setBufferNonReady();
}

void CruChannelMaster::deviceStartDma()
{
  resetCru();
  initCru();
  mLinkQueue.clear();
  mLinkSuperpageCounter = 0;
  setBufferReady();
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

  if (mLinkQueue.size() >= LINK_MAX_SUPERPAGES) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, queue is full"));
  }
  mLinkQueue.push_back(superpage);

  auto maxPages = superpage.getSize() / DMA_PAGE_SIZE;
  auto busAddress = getBusOffsetAddress(superpage.getOffset());
  getBar().pushSuperpageDescriptor(maxPages, busAddress);
}

auto CruChannelMaster::popSuperpage() -> Superpage
{
  if (getSuperpage().isReady()) {
    auto superpage = mLinkQueue.front();
    superpage.received = superpage.size;
    mLinkQueue.pop_front();
    mLinkSuperpageCounter++;
    return superpage;
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, transfer wasn't ready"));
  }
}

void CruChannelMaster::fillSuperpages()
{
  // Check for arrivals & handle them
  uint32_t superpageCount = getBar().getSuperpageCount();
  if (superpageCount > mLinkSuperpageCounter) {
    // Front superpage has arrived
    mLinkQueue.front().ready = true;
  }
}

int CruChannelMaster::getSuperpageQueueCount()
{
  return mLinkQueue.size();
}

int CruChannelMaster::getSuperpageQueueAvailable()
{
  return mLinkQueue.capacity() - mLinkQueue.size();
}

int CruChannelMaster::getSuperpageQueueCapacity()
{
  return mLinkQueue.capacity();
}

auto CruChannelMaster::getSuperpage() -> Superpage
{
  if (mLinkQueue.size() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get front superpage, queue is empty"));
  }

  return mLinkQueue.front();
}

boost::optional<float> CruChannelMaster::getTemperature()
{
  return getBar2().getTemperatureCelsius();
}

Pda::PdaBar* CruChannelMaster::getPdaBar2Ptr()
{
  if (!mPdaBar2) {
    Utilities::resetSmartPtr(mPdaBar2, getRorcDevice().getPciDevice(), 2);
  }
  return mPdaBar2.get();
}

boost::optional<std::string> CruChannelMaster::getFirmwareInfo()
{
  std::ostringstream stream;
  stream << '-' << getBar2().getFirmwareDate() << '-' << getBar2().getFirmwareTime() << std::hex
       << getBar2().getFirmwareGitHash();
  return stream.str();
}

} // namespace Rorc
} // namespace AliceO2
