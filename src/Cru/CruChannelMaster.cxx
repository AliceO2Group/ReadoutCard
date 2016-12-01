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

CruChannelMaster::CruChannelMaster(const Parameters& params)
    : ChannelMasterBase(CARD_TYPE, params, allowedChannels(), sizeof(CruFifoTable))
{
  if (getChannelParameters().dma.pageSize != DMA_PAGE_SIZE) {
    BOOST_THROW_EXCEPTION(CruException()
        << ErrorInfo::Message("CRU only supports an 8kB page size")
        << ErrorInfo::DmaPageSize(getChannelParameters().dma.pageSize));
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
  fillFifoNonLocking();
  setBufferReadyGuard();
}

/// Set up a guard object for the buffer readiness, which will set it to true when constructed (immediately), and false
/// when destructed, either explicitly in deviceStopDma(), or when CruChannelMaster is deleted.
void CruChannelMaster::setBufferReadyGuard()
{
  if (!mBufferReadyGuard) {
    Util::resetSmartPtr(mBufferReadyGuard,
        [&]{ getBar().setDataEmulatorEnabled(true); },
        [&]{ getBar().setDataEmulatorEnabled(false); });
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
  if (getPageAddresses().size() <= CRU_DESCRIPTOR_ENTRIES) {
    BOOST_THROW_EXCEPTION(CruException()
        << ErrorInfo::Message("Insufficient amount of pages fit in DMA buffer")
        << ErrorInfo::Pages(getPageAddresses().size())
        << ErrorInfo::DmaBufferSize(getChannelParameters().dma.bufferSize)
        << ErrorInfo::DmaPageSize(getChannelParameters().dma.pageSize));
  }

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
  if (getChannelParameters().generator.useDataGenerator) {
    getBar().setDataGeneratorPattern(getChannelParameters().generator.pattern);
  }

  // Status base address in the bus address space
  log((Util::getUpper32Bits(uint64_t(getFifoBus())) != 0)
        ? "Using 64-bit region for status bus address, may be unsupported by PCI/BIOS configuration"
        : "Using 32-bit region for status bus address");

  if (!Util::checkAlignment(getFifoBus(), DMA_ALIGNMENT)) {
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

int CruChannelMaster::fillFifo(int maxFill)
{
  CHANNELMASTER_LOCKGUARD();
  return fillFifoNonLocking(maxFill);
}

/// We need this because deviceStartDma() needs fillFifo() functionality, but is already a synchronized function itself.
int CruChannelMaster::fillFifoNonLocking(int maxFill)
{
  auto isArrived = [&](int descriptorIndex) {
    return getFifoUser()->statusEntries[descriptorIndex].isPageArrived();
  };

  auto resetDescriptor = [&](int descriptorIndex) {
    getFifoUser()->statusEntries[descriptorIndex].reset();
  };

  auto push = [&](int bufferIndex, int descriptorIndex) {
    auto& pageAddress = getPageAddresses()[bufferIndex];
    auto sourceAddress = reinterpret_cast<volatile void*>((descriptorIndex % NUM_OF_FW_BUFFERS) * DMA_PAGE_SIZE);
    getFifoUser()->setDescriptor(descriptorIndex, DMA_PAGE_SIZE_32, sourceAddress, pageAddress.bus);
    getBar().sendAcknowledge(); // Is this the right location..? Or should it be in the freeing?
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
