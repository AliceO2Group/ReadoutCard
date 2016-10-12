/// \file CruChannelMaster.cxx
/// \brief Implementation of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruChannelMaster.h"
#include <iostream>
#include <cassert>
#include <thread>
#include "c/rorc/rorc.h"
#include "ChannelPaths.h"
#include "RORC/Exception.h"
#include "Util.h"
#include "ChannelUtilityImpl.h"
#include "CruRegisterIndex.h"
#include "Pda/Pda.h"

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
using std::cout;
using std::endl;
using namespace std::literals;

namespace AliceO2 {
namespace Rorc {

namespace Register = CruRegisterIndex;

namespace
{
/// DMA page length in bytes
constexpr int DMA_PAGE_SIZE = 8 * 1024;
/// DMA page length in 32-bit words
constexpr int DMA_PAGE_SIZE_32 = DMA_PAGE_SIZE / 4;

constexpr int NUM_OF_BUFFERS = 32;
constexpr int FIFO_ENTRIES = 4;
constexpr int NUM_PAGES = FIFO_ENTRIES * NUM_OF_BUFFERS;

/// DMA addresses must be 32-byte aligned
constexpr uint64_t DMA_ALIGNMENT = 32;

constexpr uint32_t BUFFER_DEFAULT_VALUE = 0xCcccCccc;

} // Anonymous namespace

/// Creates a CruException and attaches data using the given message string
#define CRU_EXCEPTION(_err_message) \
  CruException() \
      << errinfo_rorc_generic_message(_err_message)

/// Amount of additional DMA buffers for this channel
static constexpr int CRU_BUFFERS_PER_CHANNEL = 0;

CruChannelMaster::CruChannelMaster(int serial, int channel)
    : ChannelMaster(CARD_TYPE, serial, channel, CRU_BUFFERS_PER_CHANNEL)
{
  constructorCommon();
}

CruChannelMaster::CruChannelMaster(int serial, int channel, const Parameters::Map& params)
    : ChannelMaster(CARD_TYPE, serial, channel, params, CRU_BUFFERS_PER_CHANNEL)
{
  constructorCommon();
}

void CruChannelMaster::constructorCommon()
{
  using Util::resetSmartPtr;

  mFifoIndexWrite = 0;
  mFifoIndexRead = 0;
  mBufferPageIndex = 0;

  mPageWasReadOut.resize(NUM_PAGES, true);
  mBufferPageIndexes.resize(CRU_DESCRIPTOR_ENTRIES, -1);

  // Initialize the page addresses
  initFifo();
  //resetBuffer();

  if (getPageAddresses().size() <= CRU_DESCRIPTOR_ENTRIES) {
    BOOST_THROW_EXCEPTION(CruException()
        << errinfo_rorc_error_message("Insufficient amount of pages fit in DMA buffer"));
  }
}

CruChannelMaster::~CruChannelMaster()
{
}

void CruChannelMaster::deviceStartDma()
{
  resetCru();
  initCru();
  setBufferReadyGuard();
}

/// Set up a guard object for the buffer readiness, which will set it to true when constructed (immediately), and false
/// when destructed, either explicitly in deviceStopDma(), or when CruChannelMaster is deleted.
void CruChannelMaster::setBufferReadyGuard()
{
  if (!mBufferReadyGuard) {
    Util::resetSmartPtr(mBufferReadyGuard,
        [&]{ setBufferReadyStatus(true); },
        [&]{ setBufferReadyStatus(false); });
  }
}

void CruChannelMaster::deviceStopDma()
{
  mBufferReadyGuard.reset(); // see setBufferReadyGuard()
}

void CruChannelMaster::resetCard(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::Nothing) {
    return;
  }

  stopDma();
  resetCru();
  startDma();
}

PageHandle CruChannelMaster::pushNextPage()
{
  if (getSharedData().mDmaState != DmaState::STARTED) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_rorc_error_message("Not in required DMA state")
        << errinfo_rorc_possible_causes({"startDma() not called"}));
  }

  // Handle for next page
  auto fifoIndex = mFifoIndexWrite;
  auto bufferIndex = mBufferPageIndex;

  // Check if page is available to write to
  if (mPageWasReadOut[fifoIndex] == false) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_rorc_error_message("Pushing page would overwrite")
        << errinfo_rorc_fifo_index(fifoIndex));
  }

  mPageWasReadOut[fifoIndex] = false;
  mBufferPageIndexes[fifoIndex] = bufferIndex;

  setDescriptor(fifoIndex, bufferIndex);

  mFifoIndexWrite = (mFifoIndexWrite + 1) % CRU_DESCRIPTOR_ENTRIES;
  mBufferPageIndex = (mBufferPageIndex + 1) % getPageAddresses().size();

  return PageHandle(fifoIndex);
}

bool CruChannelMaster::isPageArrived(const PageHandle& handle)
{
  return mFifoUser->statusEntries[handle.index].isPageArrived();
}

Page CruChannelMaster::getPage(const PageHandle& handle)
{
  return Page(getPageAddresses()[handle.index].user, DMA_PAGE_SIZE);
}

void CruChannelMaster::markPageAsRead(const PageHandle& handle)
{
  if (mPageWasReadOut[handle.index]) {
    BOOST_THROW_EXCEPTION(CruException() << errinfo_rorc_error_message("Page was already marked as read"));
  }
  mPageWasReadOut[handle.index] = true;

  acknowledgePage();
}

CardType::type CruChannelMaster::getCardType()
{
  return CardType::Cru;
}

std::vector<uint32_t> CruChannelMaster::utilityCopyFifo()
{
  std::vector<uint32_t> copy;
  auto* fifo = mFifoUser;
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
  ChannelUtility::printCruFifo(mFifoUser, os);
}

void CruChannelMaster::utilitySetLedState(bool state)
{
  int on = 0x00; // Yes, a 0 represents the on state
  int off = 0xff;
  getBarUserspace()[CruRegisterIndex::LED_STATUS] = state ? on : off;
}

void CruChannelMaster::utilitySanityCheck(std::ostream& os)
{
  ChannelUtility::cruSanityCheck(os, this);
}

void CruChannelMaster::utilityCleanupState()
{
  ChannelUtility::cruCleanupState(ChannelPaths(CARD_TYPE, getSerialNumber(), getChannelNumber()));
}

int CruChannelMaster::utilityGetFirmwareVersion()
{
  return getBarUserspace()[CruRegisterIndex::FIRMWARE_COMPILE_INFO];
}

/// Initializes the FIFO and the page addresses for it
void CruChannelMaster::initFifo()
{
  /// Amount of space reserved for the FIFO, we use multiples of the page size for uniformity
  size_t fifoSpace = ((sizeof(CruFifoTable) / DMA_PAGE_SIZE) + 1) * DMA_PAGE_SIZE;

  PageAddress fifoAddress;
  std::tie(fifoAddress, getPageAddresses()) = Pda::partitionScatterGatherList(getBufferPages().getScatterGatherList(),
      fifoSpace, DMA_PAGE_SIZE);
  mFifoUser = reinterpret_cast<CruFifoTable*>(const_cast<void*>(fifoAddress.user));
  mFifoBus = reinterpret_cast<CruFifoTable*>(const_cast<void*>(fifoAddress.bus));

  if (getPageAddresses().size() <= CRU_DESCRIPTOR_ENTRIES) {
    BOOST_THROW_EXCEPTION(CruException()
        << errinfo_rorc_error_message("Insufficient amount of pages fit in DMA buffer"));
  }

  // Initializing the descriptor table
  mFifoUser->resetStatusEntries();

  // As a safety measure, we put "valid" addresses in the descriptor table, even though we're not pushing pages yet
  // This helps prevent the card from writing to invalid addresses and crashing absolutely everything
  for (int i = 0; i < mFifoUser->descriptorEntries.size(); i++) {
    setDescriptor(i, i);
  }
}

void CruChannelMaster::setDescriptor(int pageIndex, int descriptorIndex)
{
  auto& pageAddress = getPageAddresses()[pageIndex];
  auto sourceAddress = reinterpret_cast<volatile void*>((descriptorIndex % NUM_OF_BUFFERS) * DMA_PAGE_SIZE);
  mFifoUser->setDescriptor(descriptorIndex, DMA_PAGE_SIZE_32, sourceAddress, pageAddress.bus);
}

void CruChannelMaster::resetBuffer()
{
  for (auto& page : getPageAddresses()) {
    resetPage(page.user);
  }
}

void CruChannelMaster::resetCru()
{
  bar(Register::RESET_CONTROL) = 0x2;
  std::this_thread::sleep_for(100ms);
  bar(Register::RESET_CONTROL) = 0x1;
  std::this_thread::sleep_for(100ms);
}

void CruChannelMaster::resetPage(volatile uint32_t* page)
{
  for (size_t i = 0; i < DMA_PAGE_SIZE_32; i++) {
    page[i] = BUFFER_DEFAULT_VALUE;
  }
}

void CruChannelMaster::resetPage(volatile void* page)
{
  resetPage(reinterpret_cast<volatile uint32_t*>(page));
}

volatile uint32_t& CruChannelMaster::bar(size_t index)
{
  return *(&getBarUserspace()[index]);
}

void CruChannelMaster::initCru()
{
  // Status base address in the bus address space
  if (Util::getUpper32Bits(uint64_t(mFifoBus)) != 0) {
    cout << "Warning: using 64-bit region for status bus address (" << reinterpret_cast<volatile void*>(mFifoBus)
        << "), may be unsupported by PCI/BIOS configuration.\n";
  } else {
    cout << "Info: using 32-bit region for status bus address (" << reinterpret_cast<volatile void*>(mFifoBus) << ")\n";
  }
  cout << "Info: status user address (" << reinterpret_cast<volatile void*>(mFifoUser) << ")\n";

  if (!Util::checkAlignment(mFifoBus, DMA_ALIGNMENT)) {
    BOOST_THROW_EXCEPTION(CruException() << errinfo_rorc_error_message("mFifoDevice not 32 byte aligned"));
  }

  bar(Register::STATUS_BASE_BUS_HIGH) = Util::getUpper32Bits(uint64_t(mFifoBus));
  bar(Register::STATUS_BASE_BUS_LOW) = Util::getLower32Bits(uint64_t(mFifoBus));

  // TODO Note: this stuff will be set by firmware in the future
  {
    // Status base address in the card's address space
    bar(Register::STATUS_BASE_CARD_HIGH) = 0x0;
    bar(Register::STATUS_BASE_CARD_LOW) = 0x8000;

    // Set descriptor table size (must be size - 1)
    bar(Register::DESCRIPTOR_TABLE_SIZE) = NUM_PAGES - 1;

    // Send command to the DMA engine to write to every status entry, not just the final one
    bar(Register::DONE_CONTROL) = 0x1;
  }
}

void CruChannelMaster::setBufferReadyStatus(bool ready)
{
  bar(Register::DATA_EMULATOR_CONTROL) = ready ? 0x3 : 0x0;
}

void CruChannelMaster::acknowledgePage()
{
  bar(Register::DMA_COMMAND) = 0x1;
}

} // namespace Rorc
} // namespace AliceO2
