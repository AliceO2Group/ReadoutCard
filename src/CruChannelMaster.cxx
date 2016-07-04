///
/// \file CruChannelMaster.cxx
/// \author Pascal Boeschoten
///

#include "CruChannelMaster.h"
#include <iostream>
#include "c/interface/header.h"
#include "c/rorc/rorc.h"
#include "ChannelPaths.h"

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

static constexpr int CRORC_BUFFERS_PER_CHANNEL = 2;
static constexpr int BUFFER_INDEX_FIFO = 1;

CruChannelMaster::CruChannelMaster(int serial, int channel, const ChannelParameters& params)
: ChannelMaster(serial, channel, params, CRORC_BUFFERS_PER_CHANNEL),
  mappedFileFifo(
      ChannelPaths::fifo(serial, channel).c_str()),
  bufferFifo(
      pdaDevice.getPciDevice(),
      mappedFileFifo.getAddress(),
      mappedFileFifo.getSize(),
      getBufferId(BUFFER_INDEX_FIFO)),
  crorcSharedData(
      ChannelPaths::state(serial, channel),
      sharedDataSize(),
      crorcSharedDataName(),
      FileSharedObject::find_or_construct)
{
  // Initialize (if needed) the shared data
  const auto& csd = crorcSharedData.get();

  if (csd->initializationState == InitializationState::INITIALIZED) {
    cout << "CRU shared channel state already initialized" << endl;
  } else {
    if (csd->initializationState == InitializationState::UNKNOWN) {
      cout << "Warning: unknown CRU shared channel state. Proceeding with initialization" << endl;
    }
    cout << "Initializing CRU shared channel state" << endl;
    csd->initialize();

    cout << "Clearing FIFO" << endl;
    auto& fifo = mappedFileFifo.get();

    for (size_t i = 0; i < DESCRIPTOR_ENTRIES; ++i) {
      fifo->statusEntries[i].status = 0;
    }

    for (size_t i = 0; i < DESCRIPTOR_ENTRIES; ++i) {
      auto& e = fifo->descriptorEntries[i];
      e.ctrl = (i << 18) + (params.dma.pageSize / 4);
      e.srcLow = i * params.dma.pageSize;
      e.srcHigh = 0;
      e.dstLow = 0; // TODO
      e.dstHigh = 0; // TODO
      e.reserved1 = 0;
      e.reserved2 = 0;
      e.reserved3 = 0;
    }
  }

  // Initialize the page addresses
  for (auto& entry : bufferPages.getScatterGatherList()) {
    // How many pages fit in this SGL entry
    int64_t pagesInSglEntry = entry.size / params.dma.pageSize;

    for (int64_t i = 0; i < pagesInSglEntry; ++i) {
      int64_t offset = i * params.dma.pageSize;
      PageAddress pa;
      pa.bus = (void*) ((char*) entry.addressBus) + offset;
      pa.user = (void*) ((char*) entry.addressUser) + offset;
      pageAddresses.push_back(pa);
    }
  }

  if (pageAddresses.size() <= CRORC_NUMBER_OF_PAGES) {
    ALICEO2_RORC_THROW_EXCEPTION("Insufficient amount of pages fit in DMA buffer");
  }

  // Reset page read status
  pageWasReadOut.resize(params.fifo.entries, true);
}

CruChannelMaster::~CruChannelMaster()
{
  // TODO
}

CruChannelMaster::CrorcSharedData::CrorcSharedData()
    : initializationState(InitializationState::UNKNOWN), fifoIndexWrite(0), fifoIndexRead(0), loopPerUsec(
        0), pciLoopPerUsec(0), pageIndex(0)
{
}

void CruChannelMaster::CrorcSharedData::initialize()
{
  fifoIndexWrite = 0;
  fifoIndexRead = 0;
  pageIndex = 0;
  initializationState = InitializationState::INITIALIZED;
}

void CruChannelMaster::deviceStartDma()
{
  auto& params = getParams();

  auto bar = pdaBar.getUserspaceAddressU32();
  void* fifoTableAddress = bufferFifo.getScatterGatherList()[0].addressBus;

  // Status base address, low and high part, respectively
  bar[0] = (uint64_t)fifoTableAddress & 0xffffffff;
  bar[1] = (uint64_t)fifoTableAddress >> 32;

  // Destination (card's memory) addresses, low and high part, respectively
  bar[2] = 0x8000;
  bar[3] = 0x0;

  // Set descriptor table size, same as number of available pages-1
  bar[5] = FIFO_ENTRIES-1;

  // Number of available pages-1
  bar[4] = FIFO_ENTRIES-1;

  // Give PCIe ready signal
  bar[129] = 0x1;

  // Programming the user module to trigger the data emulator
  bar[128] = 0x1;

  // Set status to send status for every page not only for the last one
  bar[6] = 0x1;
}

void CruChannelMaster::deviceStopDma()
{
  // TODO
}

void CruChannelMaster::resetCard(ResetLevel::type resetLevel)
{
  // TODO
}

ChannelMasterInterface::PageHandle CruChannelMaster::pushNextPage()
{
  // TODO
}

ChannelMaster::DataArrivalStatus::type CruChannelMaster::dataArrived(int index)
{
  // TODO
}

bool CruChannelMaster::isPageArrived(const PageHandle& handle)
{
  return dataArrived(handle.index) != DataArrivalStatus::NONE_ARRIVED;
}

} // namespace Rorc
} // namespace AliceO2
