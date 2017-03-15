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

constexpr int FIFO_FW_ENTRIES = 4; ///< The firmware works in blocks of 4 pages
constexpr int NUM_OF_FW_BUFFERS = 32; ///< ... And as such has 32 "buffers" in the FIFO
constexpr int NUM_PAGES = FIFO_FW_ENTRIES * NUM_OF_FW_BUFFERS; ///<... For a total number of 128 pages
static_assert(NUM_PAGES == CRU_DESCRIPTOR_ENTRIES, "");

/// DMA addresses must be 32-byte aligned
constexpr uint64_t DMA_ALIGNMENT = 32;

} // Anonymous namespace

CruChannelMaster::CruChannelMaster(const Parameters& parameters)
    : ChannelMasterPdaBase(parameters, allowedChannels(), sizeof(CruFifoTable)), //
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
  /// TODO XXX TODO initialize configuration params

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

  if (auto pattern = parameters.getGeneratorPattern()) {
    if (pattern != GeneratorPattern::Incremental) {
      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("CRU currently only supports 'incremental' data generator pattern")
          << ErrorInfo::GeneratorPattern(*pattern));
    }
  }

  initFifo();
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
}

/// Set buffer to ready
void CruChannelMaster::setBufferReady()
{
  if (!mBufferReady) {
    mBufferReady = true;
    getBar().setDataEmulatorEnabled(true);
    std::this_thread::sleep_for(10ms);
  }
}

/// Set buffer to non-ready
void CruChannelMaster::setBufferNonReady()
{
  if (mBufferReady) {
    mBufferReady = false;
    getBar().setDataEmulatorEnabled(false);
  }
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

/// Initializes the FIFO and the page addresses for it
void CruChannelMaster::initFifo()
{
  getFifoUser()->resetStatusEntries();
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

  // Status base address in the bus address space
  log((Utilities::getUpper32Bits(uint64_t(getFifoBus())) != 0)
        ? "Using 64-bit region for status bus address, may be unsupported by PCI/BIOS configuration"
        : "Using 32-bit region for status bus address");

  if (!Utilities::checkAlignment(getFifoBus(), DMA_ALIGNMENT)) {
    BOOST_THROW_EXCEPTION(CruException() << ErrorInfo::Message("FIFO bus address not 32 byte aligned"));
  }

  getBar().setFifoBusAddress(getFifoBus());

  // TODO Note: this stuff will be set by firmware in the future
  {
    // Status base address in the card's address space
    getBar().setFifoCardAddress();

    // Set descriptor table size (must be size - 1)
    getBar().setDescriptorTableSize();

    // Send command to the DMA engine to write to every status entry, not just the final one
    getBar().setDoneControl();
  }
}

std::vector<uint32_t> CruChannelMaster::utilityCopyFifo()
{
  std::vector<uint32_t> copy;
  auto* fifo = getFifoUser();
  size_t size = sizeof(std::decay<decltype(fifo)>::type);
  size_t elements = size / sizeof(decltype(copy)::value_type);
  copy.reserve(elements);

  auto* fifoData = reinterpret_cast<char*>(fifo);
  auto* copyData = reinterpret_cast<char*>(copy.data());
  std::copy(fifoData, fifoData + size, copyData);
  return copy;
}

void CruChannelMaster::utilityPrintFifo(std::ostream& os)
{
  ChannelUtility::printCruFifo(getFifoUser(), os);
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
  // Push new pages into superpage
  if (!mSuperpageQueue.getPushing().empty()) {
    SuperpageQueueEntry& entry = mSuperpageQueue.getPushingFrontEntry();

    int freeDescriptors = FIFO_QUEUE_MAX - mFifoSize;
    int freePages = entry.getUnpushedPages();
    int possibleToPush = std::min(freeDescriptors, freePages);

    for (int i = 0; i < possibleToPush; ++i) {
      pushIntoSuperpage(entry);
    }

    if (mFifoSize >= READYFIFO_ENTRIES) {
      // We should only enable the buffer when all the descriptors are filled, because the card may use them all as soon
      // as the ready signal is given
      setBufferReady();
    }

    if (entry.isPushed()) {
      // Remove superpage from pushing queue
      mSuperpageQueue.removeFromPushingQueue();
    }
  }

  // Check for arrivals & handle them
  if (!mSuperpageQueue.getArrivals().empty()) {
    auto isArrived = [&](int descriptorIndex) { return getFifoUser()->statusEntries[descriptorIndex].isPageArrived(); };
    auto resetDescriptor = [&](int descriptorIndex) { getFifoUser()->statusEntries[descriptorIndex].reset(); };

    while (mFifoSize > 0) {
      SuperpageQueueEntry& entry = mSuperpageQueue.getArrivalsFrontEntry();

      if (isArrived(mFifoBack)) {
        resetDescriptor(mFifoBack);
        mFifoSize--;
        mFifoBack = (mFifoBack + 1) % READYFIFO_ENTRIES;
        entry.superpage.received += DMA_PAGE_SIZE;

        if (entry.superpage.isFilled()) {
          // Move superpage to filled queue
          mSuperpageQueue.moveFromArrivalsToFilledQueue();
        }
      } else {
        // If the back one hasn't arrived yet, the next ones will certainly not have arrived either...
        break;
      }
    }
  }
}

void CruChannelMaster::pushIntoSuperpage(SuperpageQueueEntry& superpage)
{
  assert(mFifoSize < FIFO_QUEUE_MAX);
  assert(superpage.pushedPages < superpage.status.maxPages);

  auto descriptorIndex = getFifoFront();
  uintptr_t sourceAddress = (descriptorIndex % NUM_OF_FW_BUFFERS) * DMA_PAGE_SIZE;
  uintptr_t pageBusAddress = getNextSuperpageBusAddress(superpage);

  getFifoUser()->setDescriptor(descriptorIndex, DMA_PAGE_SIZE_32, sourceAddress, pageBusAddress);

  if (mBufferReady) {
    getBar().sendAcknowledge();
  }

  mFifoSize++;
  superpage.pushedPages++;
}

uintptr_t CruChannelMaster::getNextSuperpageBusAddress(const SuperpageQueueEntry& superpage)
{
  return superpage.busAddress + DMA_PAGE_SIZE * superpage.pushedPages;
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


} // namespace Rorc
} // namespace AliceO2
