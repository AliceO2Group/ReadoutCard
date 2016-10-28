/// \file CruChannelMaster.cxx
/// \brief Implementation of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruChannelMaster.h"
#include <boost/dynamic_bitset.hpp>
#include <thread>
#include "ChannelPaths.h"
#include "ChannelUtilityImpl.h"
#include "CruRegisterIndex.h"
#include "Pda/Pda.h"
#include "RORC/Exception.h"
#include "Util.h"

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
constexpr int DMA_PAGE_SIZE = 8 * 1024;
/// DMA page length in 32-bit words
constexpr int DMA_PAGE_SIZE_32 = DMA_PAGE_SIZE / 4;

constexpr int FIFO_FW_ENTRIES = 4; ///< The firmware works in blocks of 4 pages
constexpr int NUM_OF_FW_BUFFERS = 32; ///< ... And as such has 32 "buffers" in the FIFO
constexpr int NUM_PAGES = FIFO_FW_ENTRIES * NUM_OF_FW_BUFFERS; ///<... For a total number of 128 pages
static_assert(NUM_PAGES == CRU_DESCRIPTOR_ENTRIES, "");

/// DMA addresses must be 32-byte aligned
constexpr uint64_t DMA_ALIGNMENT = 32;

/// Amount of additional DMA buffers for this channel
static constexpr int CRU_BUFFERS_PER_CHANNEL = 0;

} // Anonymous namespace

CruChannelMaster::CruChannelMaster(const Parameters& params)
    : ChannelMaster(CARD_TYPE, params, CRU_BUFFERS_PER_CHANNEL, allowedChannels())
{
  if (getChannelParameters().dma.pageSize != DMA_PAGE_SIZE) {
    BOOST_THROW_EXCEPTION(CruException()
        << errinfo_rorc_error_message("CRU only supports an 8kB page size")
        << errinfo_rorc_dma_page_size(getChannelParameters().dma.pageSize));
  }

  initFifo();
  mPageManager.setAmountOfPages(getPageAddresses().size());
}

auto CruChannelMaster::allowedChannels() -> AllowedChannels {
  // Note: BAR 1 is not available because BAR 0 is 64-bit wide, so it 'consumes' two BARs.
  return {0, 2};
}

CruChannelMaster::~CruChannelMaster()
{
}

void CruChannelMaster::deviceStartDma()
{
  resetCru();
  initCru();
  // Push initial 128 pages
  fillFifo();
  setBufferReadyGuard();
}

/// Set up a guard object for the buffer readiness, which will set it to true when constructed (immediately), and false
/// when destructed, either explicitly in deviceStopDma(), or when CruChannelMaster is deleted.
void CruChannelMaster::setBufferReadyGuard()
{
  if (!mBufferReadyGuard) {
    Util::resetSmartPtr(mBufferReadyGuard,
        [&]{ bar(Register::DATA_EMULATOR_CONTROL) = 0x3; },
        [&]{ bar(Register::DATA_EMULATOR_CONTROL) = 0x0; });
  }
}

void CruChannelMaster::deviceStopDma()
{
  mBufferReadyGuard.reset(); // see setBufferReadyGuard()
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
  /// Amount of space reserved for the FIFO, we use multiples of the page size for uniformity
  size_t fifoSpace = ((sizeof(CruFifoTable) / DMA_PAGE_SIZE) + 1) * DMA_PAGE_SIZE;

  PageAddress fifoAddress;
  std::tie(fifoAddress, getPageAddresses()) = Pda::partitionScatterGatherList(getBufferPages().getScatterGatherList(),
      fifoSpace, DMA_PAGE_SIZE);
  mFifoUser = reinterpret_cast<CruFifoTable*>(const_cast<void*>(fifoAddress.user));
  mFifoBus = reinterpret_cast<CruFifoTable*>(const_cast<void*>(fifoAddress.bus));

  if (getPageAddresses().size() <= CRU_DESCRIPTOR_ENTRIES) {
    BOOST_THROW_EXCEPTION(CruException()
        << errinfo_rorc_error_message("Insufficient amount of pages fit in DMA buffer")
        << errinfo_rorc_pages(getPageAddresses().size())
        << errinfo_rorc_dma_buffer_size(getChannelParameters().dma.bufferSize)
        << errinfo_rorc_dma_page_size(getChannelParameters().dma.pageSize));
  }

  mFifoUser->resetStatusEntries();
}

void CruChannelMaster::resetCru()
{
  bar(Register::RESET_CONTROL) = 0x2;
  std::this_thread::sleep_for(100ms);
  bar(Register::RESET_CONTROL) = 0x1;
  std::this_thread::sleep_for(100ms);
}

void CruChannelMaster::initCru()
{
  // Status base address in the bus address space
  if (Util::getUpper32Bits(uint64_t(mFifoBus)) != 0) {
    // TODO InfoLogger
    //cout << "Info: using 64-bit region for status bus address, may be unsupported by PCI/BIOS configuration.\n";
  } else {
    // TODO InfoLogger
    //cout << "Info: using 32-bit region for status bus address\n";
  }

  if (!Util::checkAlignment(mFifoBus, DMA_ALIGNMENT)) {
    BOOST_THROW_EXCEPTION(CruException() << errinfo_rorc_error_message("FIFO bus address not 32 byte aligned"));
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

int CruChannelMaster::fillFifo(int maxFill)
{
  CHANNELMASTER_LOCKGUARD();

  auto isArrived = [&](int descriptorIndex) {
    return mFifoUser->statusEntries[descriptorIndex].isPageArrived();
  };

  auto resetDescriptor = [&](int descriptorIndex) {
    mFifoUser->statusEntries[descriptorIndex].reset();
  };

  auto push = [&](int bufferIndex, int descriptorIndex) {
    auto& pageAddress = getPageAddresses()[bufferIndex];
    auto sourceAddress = reinterpret_cast<volatile void*>((descriptorIndex % NUM_OF_FW_BUFFERS) * DMA_PAGE_SIZE);
    mFifoUser->setDescriptor(descriptorIndex, DMA_PAGE_SIZE_32, sourceAddress, pageAddress.bus);
    bar(Register::DMA_COMMAND) = 0x1; // Is this the right location..? Or should it be in the freeing?
  };

  mPageManager.handleArrivals(isArrived, resetDescriptor);
  int pushCount = mPageManager.pushPages(maxFill, push);
  return pushCount;
}

int CruChannelMaster::getAvailableCount()
{
  CHANNELMASTER_LOCKGUARD();

  return mPageManager.getArrivedCount();
}

auto CruChannelMaster::popPageInternal(const MasterSharedPtr& channel) -> std::shared_ptr<Page>
{
  CHANNELMASTER_LOCKGUARD();

  if (auto page = mPageManager.useArrivedPage()) {
    int bufferIndex = *page;
    return std::make_shared<Page>(getPageAddresses()[bufferIndex].user, DMA_PAGE_SIZE, bufferIndex, channel);
  }
  return nullptr;
}

void CruChannelMaster::freePage(const Page& page)
{
  CHANNELMASTER_LOCKGUARD();

  mPageManager.freePage(page.getId());
}

volatile uint32_t& CruChannelMaster::bar(size_t index)
{
  return *(&getBarUserspace()[index]);
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

} // namespace Rorc
} // namespace AliceO2
