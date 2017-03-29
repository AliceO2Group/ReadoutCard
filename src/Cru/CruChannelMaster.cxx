/// \file CruChannelMaster.cxx
/// \brief Implementation of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruChannelMaster.h"
#include <boost/dynamic_bitset.hpp>
#include <thread>
#include "ChannelPaths.h"
#include "ChannelUtilityImpl.h"
#include "ExceptionInternal.h"
#include "Utilities/SmartPointer.h"

/// Creates a CruException and attaches data using the given message string
#define CRU_EXCEPTION(_err_message) \
  CruException() \
      << errinfo_rorc_generic_message(_err_message)

using namespace std::literals;

namespace AliceO2 {
namespace Rorc {

namespace Register = CruRegisterIndex;

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
    : ChannelMasterPdaBase(parameters, allowedChannels(), sizeof(0)), //
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
  mSuperpageQueue.clear();
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

  SuperpageQueueEntry entry;
  entry.busAddress = getBusOffsetAddress(superpage.getOffset());
  entry.maxPages = superpage.getSize() / DMA_PAGE_SIZE;
  entry.pushedPages = 0;
  entry.superpage = superpage;
  entry.superpage.received = 0;

  mSuperpageQueue.addToQueue(entry);
}

auto CruChannelMaster::popSuperpage() -> Superpage
{
  return mSuperpageQueue.removeFromFilledQueue().superpage;
}

void CruChannelMaster::fillSuperpages()
{
  // Push superpages
  {
    size_t freeDescriptors = FIFO_QUEUE_MAX - mFifoSize;
    int possibleToPush = std::min(mSuperpageQueue.getPushing().size(), freeDescriptors);

    for (int i = 0; i < possibleToPush; ++i) {
      SuperpageQueueEntry& entry = mSuperpageQueue.getPushingFrontEntry();
      pushSuperpage(entry);
      // Remove superpage from pushing queue
      mSuperpageQueue.removeFromPushingQueue();
    }
  }

  // Check for arrivals & handle them
  if (!mSuperpageQueue.getArrivals().empty()) {
    while (mFifoSize > 0) {
      SuperpageQueueEntry& entry = mSuperpageQueue.getArrivalsFrontEntry();
      auto pagesArrived = getBar().getSuperpagePushedPages(getFifoBack());
      assert(pagesArrived <= entry.maxPages);
      entry.superpage.received = pagesArrived * DMA_PAGE_SIZE;

      if (pagesArrived == entry.maxPages) {
        incrementFifoBack();
        mSuperpageQueue.moveFromArrivalsToFilledQueue();
      } else {
        // If the back one hasn't arrived yet, the next ones will certainly not have arrived either...
        break;
      }
    }
  }
}

void CruChannelMaster::pushSuperpage(SuperpageQueueEntry& entry)
{
  assert(mFifoSize < FIFO_QUEUE_MAX);
  assert(entry.pushedPages < entry.maxPages);

  auto descriptorIndex = getFifoFront();

  getBar().setSuperpageDescriptor(descriptorIndex, entry.maxPages, entry.busAddress);

  incrementFifoFront();
  entry.pushedPages = entry.maxPages;
}

int CruChannelMaster::getSuperpageQueueCount()
{
  return mSuperpageQueue.getQueueCount();
}

int CruChannelMaster::getSuperpageQueueAvailable()
{
  return mSuperpageQueue.getQueueAvailable();
}

int CruChannelMaster::getSuperpageQueueCapacity()
{
  return mSuperpageQueue.getQueueCapacity();
}

auto CruChannelMaster::getSuperpage() -> Superpage
{
  return mSuperpageQueue.getFrontSuperpage();
}

boost::optional<float> CruChannelMaster::getTemperature()
{
  return {};
  //return getBar2().getTemperatureCelsius(); // Note: disabled until new FW is in place. Using this may crash the machine
}

Pda::PdaBar* CruChannelMaster::getPdaBar2Ptr()
{
  if (!mPdaBar2) {
    Utilities::resetSmartPtr(mPdaBar2, getRorcDevice().getPciDevice(), 2);
  }
  return mPdaBar2.get();
}

std::vector<uint32_t> CruChannelMaster::utilityCopyFifo()
{
//  std::vector<uint32_t> copy;
//  auto* fifo = getFifoUser();
//  size_t size = sizeof(std::decay<decltype(fifo)>::type);
//  size_t elements = size / sizeof(decltype(copy)::value_type);
//  copy.reserve(elements);
//
//  auto* fifoData = reinterpret_cast<char*>(fifo);
//  auto* copyData = reinterpret_cast<char*>(copy.data());
//  std::copy(fifoData, fifoData + size, copyData);
//  return copy;
  return {};
}

void CruChannelMaster::utilityPrintFifo(std::ostream& os)
{
//  ChannelUtility::printCruFifo(getFifoUser(), os);
}

void CruChannelMaster::utilitySetLedState(bool state)
{
  getBar().setLedState(state);
}

void CruChannelMaster::utilitySanityCheck(std::ostream& os)
{
  ChannelUtility::cruSanityCheck(os, this);
}

void CruChannelMaster::utilityCleanupState()
{
  ChannelUtility::cruCleanupState(ChannelPaths(getCardDescriptor().pciAddress, getChannelNumber()));
}

int CruChannelMaster::utilityGetFirmwareVersion()
{
  return getBar().getFirmwareCompileInfo();
}

} // namespace Rorc
} // namespace AliceO2
