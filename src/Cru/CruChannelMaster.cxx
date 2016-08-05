///
/// \file CruChannelMaster.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

// XXX Note: this class is very under construction

#include "CruChannelMaster.h"
#include <iostream>
#include <cassert>
#include "c/rorc/rorc.h"
#include "ChannelPaths.h"
#include "RorcException.h"
#include "Util.h"
#include "ChannelUtilityImpl.h"
#include "CruRegisterIndex.h"

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

/// Creates a CruException and attaches data using the given message string
#define CRU_EXCEPTION(_err_message) \
  CruException() \
      << errinfo_rorc_generic_message(_err_message)

/// Throws a CruException with the given message string
#define THROW_CRU_EXCEPTION(_err_message) \
  BOOST_THROW_EXCEPTION(CruException() \
      << errinfo_rorc_generic_message(_err_message))

/// Amount of additional DMA buffers for this channel
static constexpr int CRU_BUFFERS_PER_CHANNEL = 1;

/// The index of the DMA buffer for the FIFO
static constexpr int BUFFER_INDEX_FIFO = 1;

uint32_t getLower32Bits(uint64_t x)
{
  return x & 0xFfffFfff;
}

uint32_t getUpper32Bits(uint64_t x)
{
  return (x >> 32) & 0xFfffFfff;
}

CruChannelMaster::CruChannelMaster(int serial, int channel)
    : ChannelMaster(serial, channel, CRU_BUFFERS_PER_CHANNEL)
{
  constructorCommon();
}

CruChannelMaster::CruChannelMaster(int serial, int channel, const ChannelParameters& params)
    : ChannelMaster(serial, channel, params, CRU_BUFFERS_PER_CHANNEL)
{
  constructorCommon();
}

void CruChannelMaster::constructorCommon()
{
  using Util::resetSmartPtr;

  resetSmartPtr(mappedFileFifo, ChannelPaths::fifo(getSerialNumber(), getChannelNumber()).c_str());

  resetSmartPtr(bufferFifo, getRorcDevice().getPciDevice(), mappedFileFifo->getAddress(), mappedFileFifo->getSize(),
      getBufferId(BUFFER_INDEX_FIFO));

  resetSmartPtr(cruSharedData, ChannelPaths::state(getSerialNumber(), getChannelNumber()), sharedDataSize(),
      cruSharedDataName(), FileSharedObject::find_or_construct);

  pendingPages = 0;

  pageWasReadOut.resize(CRU_DESCRIPTOR_ENTRIES, true);

  auto& params = getParams();

  assert(params.dma.pageSize == (8 * 1024)); // Currently the only supported page size is 8 KiB

  // Initialize (if needed) the shared data
  const auto& csd = cruSharedData->get();

  if (csd->initializationState == InitializationState::INITIALIZED) {
    cout << "[LOG] CRU shared channel state already initialized" << endl;
  } else {
    if (csd->initializationState == InitializationState::UNKNOWN) {
      cout << "[LOG] Warning: unknown CRU shared channel state. Proceeding with initialization" << endl;
    }
    cout << "[LOG] Initializing CRU shared channel state" << endl;
    csd->initialize();

    cout << "[LOG] Clearing FIFO" << endl;
    auto fifo = mappedFileFifo->get();

    for (size_t i = 0; i < fifo->statusEntries.size(); ++i) {
      fifo->statusEntries[i].status = 0;
    }
  }

  // Initialize the page addresses
  for (auto& entry : getBufferPages().getScatterGatherList()) {
    // How many pages fit in this SGL entry
    int64_t pagesInSglEntry = entry.size / params.dma.pageSize;

    for (int64_t i = 0; i < pagesInSglEntry; ++i) {
      int64_t offset = i * params.dma.pageSize;
      PageAddress pa;
      pa.bus = (void*) (((char*) entry.addressBus) + offset);
      pa.user = (void*) (((char*) entry.addressUser) + offset);
      getPageAddresses().push_back(pa);
    }
  }

  if (getPageAddresses().size() <= CRU_DESCRIPTOR_ENTRIES) {
    THROW_CRU_EXCEPTION("Insufficient amount of pages fit in DMA buffer");
  }
}

CruChannelMaster::~CruChannelMaster()
{
  // TODO
}

CruChannelMaster::CruSharedData::CruSharedData()
    : initializationState(InitializationState::UNKNOWN), fifoIndexWrite(0), fifoIndexRead(0), pageIndex(0)
{
}

void CruChannelMaster::CruSharedData::initialize()
{
  initializationState = InitializationState::INITIALIZED;
  fifoIndexWrite = 0;
  fifoIndexRead = 0;
  pageIndex = 0;
}

void CruChannelMaster::deviceStartDma()
{
  using namespace CruRegisterIndex;
  uint64_t fifoTableAddress = (uint64_t) bufferFifo->getScatterGatherList()[0].addressBus;
  auto bar = getBarUserspace();
  bar[STATUS_BASE_BUS_LOW] = getLower32Bits(fifoTableAddress);
  bar[STATUS_BASE_BUS_HIGH] = getUpper32Bits(fifoTableAddress);
  bar[FIFO_BASE_CARD_LOW] = 0x8000;
  bar[FIFO_BASE_CARD_HIGH] = 0x0;
  bar[START_DMA] = CRU_DESCRIPTOR_ENTRIES - 1;
  bar[DESCRIPTOR_TABLE_SIZE] = CRU_DESCRIPTOR_ENTRIES - 1;
  bar[PCIE_READY] = 0x1;
  bar[DATA_EMULATOR_ENABLE] = 0x1;
  bar[SEND_STATUS] = 0x1;
}

void CruChannelMaster::deviceStopDma()
{
  // TODO Not sure if this is the correct way

  // Set status to send status for every page not only for the last one
  getBarUserspace()[CruRegisterIndex::SEND_STATUS] = 0x0;
}

void CruChannelMaster::resetCard(ResetLevel::type)
{
  // TODO
}

ChannelMasterInterface::PageHandle CruChannelMaster::pushNextPage()
{
  auto handle = ChannelMasterInterface::PageHandle(pendingPages);

  if (pendingPages < 128) {
    // Wait until we have 128 pages
    pendingPages++;
    return handle;
  } else {
    // Actually push pages
    auto pageSize = getParams().dma.pageSize;
    auto fifo = mappedFileFifo->get();

    for (size_t i = 0; i < fifo->statusEntries.size(); ++i) {
      fifo->statusEntries[i].status = 0;
    }

    for (size_t i = 0; i < fifo->statusEntries.size(); ++i) {
      auto& e = fifo->descriptorEntries[i];

      // Adresses in the card's memory (DMA source)
      e.srcLow = i * pageSize;
      e.srcHigh = 0x0;

      // Addresses in the RAM (DMA destination)
      auto busAddress = (uint64_t) getPageAddresses()[0].bus;
      e.dstLow = getLower32Bits(busAddress);
      e.dstHigh = getUpper32Bits(busAddress);

      // Page size
      e.ctrl = (i << 18) + (pageSize / 4);

      // Fill the reserved bit with zero
      e.reserved1 = 0x0;
      e.reserved2 = 0x0;
      e.reserved3 = 0x0;
    }

    pendingPages = 0;
    return handle;
  }
}

bool CruChannelMaster::isPageArrived(const PageHandle&)
{
  // TODO
  return false;
}

Page CruChannelMaster::getPage(const PageHandle& handle)
{
  return Page(getPageAddresses()[handle.index].user);
}

void CruChannelMaster::markPageAsRead(const PageHandle& handle)
{
  if (pageWasReadOut[handle.index]) {
    THROW_CRU_EXCEPTION("Page was already marked as read");
  }
  pageWasReadOut[handle.index] = true;
}

CardType::type CruChannelMaster::getCardType()
{
  return CardType::CRU;
}

std::vector<uint32_t> CruChannelMaster::utilityCopyFifo()
{
  ALICEO2_RORC_THROW_EXCEPTION("Not implemented");
  return std::vector<uint32_t>();
}

void CruChannelMaster::utilityPrintFifo(std::ostream& os)
{
  ChannelUtility::printCruFifo(mappedFileFifo->get(), os);
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
  ChannelUtility::cruCleanupState(getSerialNumber(), getChannelNumber());
}

int CruChannelMaster::utilityGetFirmwareVersion()
{
  return getBarUserspace()[CruRegisterIndex::FIRMWARE_COMPILE_INFO];
}

} // namespace Rorc
} // namespace AliceO2
