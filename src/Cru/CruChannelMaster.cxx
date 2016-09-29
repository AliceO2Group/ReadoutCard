/// \file CruChannelMaster.cxx
/// \brief Implementation of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruChannelMaster.h"
#include <iostream>
#include <cassert>
#include "c/rorc/rorc.h"
#include "ChannelPaths.h"
#include "RORC/Exception.h"
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

/// Amount of additional DMA buffers for this channel
static constexpr int CRU_BUFFERS_PER_CHANNEL = 1;

/// The index of the DMA buffer for the FIFO
static constexpr int BUFFER_INDEX_FIFO = 1;

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

  ChannelPaths paths(CARD_TYPE, getSerialNumber(), getChannelNumber());
  resetSmartPtr(mMappedFileFifo, paths.fifo().c_str());

  resetSmartPtr(mBufferFifo, getRorcDevice().getPciDevice(), mMappedFileFifo->getAddress(), mMappedFileFifo->getSize(),
      getBufferId(BUFFER_INDEX_FIFO));

  resetSmartPtr(mCruSharedData, paths.state(), getSharedDataSize(), getCruSharedDataName().c_str(),
      FileSharedObject::find_or_construct);

  mPendingPages = 0;

  mPageWasReadOut.resize(CRU_DESCRIPTOR_ENTRIES, true);

  auto& params = getParams();

  assert(params.dma.pageSize == (8 * 1024)); // Currently the only supported page size is 8 KiB

  // Initialize (if needed) the shared data
  const auto& csd = mCruSharedData->get();

  if (csd->mInitializationState == InitializationState::INITIALIZED) {
    cout << "[LOG] CRU shared channel state already initialized" << endl;
  } else {
    if (csd->mInitializationState == InitializationState::UNKNOWN) {
      cout << "[LOG] Warning: unknown CRU shared channel state. Proceeding with initialization" << endl;
    }
    cout << "[LOG] Initializing CRU shared channel state" << endl;
    csd->initialize();

    cout << "[LOG] Clearing FIFO" << endl;
    auto fifo = mMappedFileFifo->get();

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
    BOOST_THROW_EXCEPTION(CruException()
        << errinfo_rorc_error_message("Insufficient amount of pages fit in DMA buffer"));
  }
}

CruChannelMaster::~CruChannelMaster()
{
}

CruChannelMaster::SharedData::SharedData()
    : mInitializationState(InitializationState::UNKNOWN), mFifoIndexWrite(0), mFifoIndexRead(0), mPageIndex(0)
{
}

void CruChannelMaster::SharedData::initialize()
{
  mInitializationState = InitializationState::INITIALIZED;
  mFifoIndexWrite = 0;
  mFifoIndexRead = 0;
  mPageIndex = 0;
}

void CruChannelMaster::deviceStartDma()
{
  using namespace CruRegisterIndex;
  uint64_t fifoTableAddress = (uint64_t) mBufferFifo->getScatterGatherList()[0].addressBus;
  auto bar = getBarUserspace();
  bar[STATUS_BASE_BUS_LOW] = Util::getLower32Bits(fifoTableAddress);
  bar[STATUS_BASE_BUS_HIGH] = Util::getUpper32Bits(fifoTableAddress);
  bar[STATUS_BASE_CARD_LOW] = 0x8000;
  bar[STATUS_BASE_CARD_HIGH] = 0x0;
  bar[DMA_POINTER] = CRU_DESCRIPTOR_ENTRIES - 1;
  bar[DESCRIPTOR_TABLE_SIZE] = CRU_DESCRIPTOR_ENTRIES - 1;
  bar[SOFTWARE_BUFFER_READY] = 0x1;
  bar[DATA_EMULATOR_CONTROL] = 0x1;
  bar[DONE_CONTROL] = 0x1;
}

void CruChannelMaster::deviceStopDma()
{
  // TODO Not sure if this is the correct way

  // Set status to send status for every page not only for the last one
  getBarUserspace()[CruRegisterIndex::DONE_CONTROL] = 0x0;
}

void CruChannelMaster::resetCard(ResetLevel::type)
{
  // TODO
}

PageHandle CruChannelMaster::pushNextPage()
{
  auto handle = PageHandle(mPendingPages);

  if (mPendingPages < 128) {
    // Wait until we have 128 pages
    mPendingPages++;
    return handle;
  } else {
    // Actually push pages
    auto pageSize = getParams().dma.pageSize;
    auto fifo = mMappedFileFifo->get();

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
      e.dstLow = Util::getLower32Bits(busAddress);
      e.dstHigh = Util::getUpper32Bits(busAddress);

      // Page size
      e.ctrl = (i << 18) + (pageSize / 4);

      // Fill the reserved bit with zero
      e.reserved1 = 0x0;
      e.reserved2 = 0x0;
      e.reserved3 = 0x0;
    }

    mPendingPages = 0;
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
  if (mPageWasReadOut[handle.index]) {
    BOOST_THROW_EXCEPTION(CruException() << errinfo_rorc_error_message("Page was already marked as read"));
  }
  mPageWasReadOut[handle.index] = true;
}

CardType::type CruChannelMaster::getCardType()
{
  return CardType::Cru;
}

std::vector<uint32_t> CruChannelMaster::utilityCopyFifo()
{
  std::vector<uint32_t> copy;
  auto* fifo = mMappedFileFifo->get();
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
  ChannelUtility::printCruFifo(mMappedFileFifo->get(), os);
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
