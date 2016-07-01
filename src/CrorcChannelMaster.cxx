#include "CrorcChannelMaster.h"
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

CrorcChannelMaster::CrorcChannelMaster(int serial, int channel, const ChannelParameters& params)
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
   cout << "CRORC shared channel state already initialized" << endl;
  }
  else {
   if (csd->initializationState == InitializationState::UNKNOWN) {
     cout << "Warning: unknown CRORC shared channel state. Proceeding with initialization" << endl;
   }
   cout << "Initializing CRORC shared channel state" << endl;
   csd->reset();

   cout << "Clearing readyFifo" << endl;
   mappedFileFifo.get()->reset();
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

CrorcChannelMaster::~CrorcChannelMaster()
{
}

CrorcChannelMaster::CrorcSharedData::CrorcSharedData()
    : initializationState(InitializationState::UNKNOWN), fifoIndexWrite(0), fifoIndexRead(0), loopPerUsec(
        0), pciLoopPerUsec(0), pageIndex(0)
{
}
void CrorcChannelMaster::CrorcSharedData::reset()
{
  fifoIndexWrite = 0;
  fifoIndexRead = 0;
  pageIndex = 0;
  initializationState = InitializationState::INITIALIZED;
}

void CrorcChannelMaster::deviceStartDma()
{
  // For random sleep time after every received data block. // TODO ...???
  if(SLEEP_TIME || LOAD_TIME){
    srand(time(NULL));
  }

  auto& params = getParams();

  // Initializing the software FIFO
  // initializeReadyFifo(cd); // Already done in constructor

  // Find DIU version, required for armDdl()
  // XXX TODO XXX This whole thing relies on global variables. Bad. Refactor.
  ddlFindDiuVersion(pdaBar.getUserspaceAddress());

  // Resetting the card,according to the RESET LEVEL parameter
  resetCard(params.initialResetLevel);

  // Setting the card to be able to receive data
  startDataReceiving();

  // Initializing the firmware FIFO, pushing (entries) pages
  initializeFreeFifo(params.fifo.entries);

  if (params.generator.useDataGenerator) {
    // Starting the data generator
    startDataGenerator(params.generator);
  } else {
    if (!getParams().noRDYRX) {
      uint64_t timeout = crorcSharedData.get()->pciLoopPerUsec * DDL_RESPONSE_TIME;
      uint64_t respCycle = crorcSharedData.get()->pciLoopPerUsec * DDL_RESPONSE_TIME;
      auto userAddress = pdaBar.getUserspaceAddressU32();

      // Clearing SIU/DIU status.
      if (rorcCheckLink(userAddress) != RORC_STATUS_OK) {
        printf("SIU not seen. Can not clear SIU status.\n");
      } else {
        if (ddlReadSiu(userAddress, 0, timeout) != -1) {
          printf("SIU status cleared.\n");
        }
      }

      if (ddlReadDiu(userAddress, 0, timeout) != -1) {
        printf("DIU status cleared.\n");
      }

      // RDYRX command to FEE
      stword_t stw;
      int returnCode = rorcStartTrigger(userAddress, respCycle, &stw);

      if (returnCode == RORC_LINK_NOT_ON) {
        ALICEO2_RORC_THROW_EXCEPTION("Error: LINK IS DOWN, RDYRX command can not be sent");
      }

      if (returnCode == RORC_STATUS_ERROR) {
        ALICEO2_RORC_THROW_EXCEPTION("Error: RDYRX command can not be sent");
      }

      if (returnCode == RORC_NOT_ACCEPTED) {
        ALICEO2_RORC_THROW_EXCEPTION(" No reply arrived for RDYRX in timeout"); // %lld usec\n", (unsigned long long) DDL_RESPONSE_TIME);
      } else {
        printf(" FEE accepted the RDYDX command. Its reply: 0x%08lx\n", stw.stw);
      }
    }
  }
}

int rorcStopDataGenerator(volatile uint32_t* buff)
{
  rorcWriteReg(buff, C_CSR, DRORC_CMD_STOP_DG);
  return (RORC_STATUS_OK);
}

void CrorcChannelMaster::deviceStopDma()
{
  auto userAddress = pdaBar.getUserspaceAddressU32();

  // Stopping receiving data
  if (getParams().generator.useDataGenerator) {
    rorcStopDataGenerator(userAddress);
    rorcStopDataReceiver(userAddress);
  } else if(getParams().noRDYRX) {
    // Sending EOBTR to FEE.
    stword_t stw;
    uint64_t timeout = crorcSharedData.get()->pciLoopPerUsec * DDL_RESPONSE_TIME;
    int returnCode = rorcStopTrigger(userAddress, timeout, &stw);

    if (returnCode == RORC_LINK_NOT_ON) {
      ALICEO2_RORC_THROW_EXCEPTION("Error: LINK IS DOWN, EOBTR command can not be sent");
    }

    if (returnCode == RORC_STATUS_ERROR) {
      ALICEO2_RORC_THROW_EXCEPTION("Error: EOBTR command can not be sent");
    }

    printf(" EOBTR command sent to the FEE\n");

    if (returnCode != RORC_NOT_ACCEPTED) {
      printf(" FEE accepted the EOBTR command. Its reply: 0x%08lx\n", stw.stw);
    }
  }
}

void CrorcChannelMaster::resetCard(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::NOTHING) {
    return;
  }

  auto loopbackMode = getParams().generator.loopbackMode;

  if (resetLevel == ResetLevel::RORC_ONLY) {
    armDdl(RORC_RESET_RORC);
  }

  if (LoopbackMode::isExternal(loopbackMode)) {
    armDdl(RORC_RESET_DIU);

    if ((resetLevel == ResetLevel::RORC_DIU_SIU) && (loopbackMode != LoopbackMode::EXTERNAL_DIU))
    {
      // Wait a little before SIU reset.
      usleep(100000);
      // Reset SIU.
      armDdl(RORC_RESET_SIU);
      armDdl(RORC_RESET_DIU);
    }

    armDdl(RORC_RESET_RORC);
  }

  // Wait a little after reset.
  usleep(100000);
}

void CrorcChannelMaster::armDdl(int resetMask)
{
  if (rorcArmDDL(pdaBar.getUserspaceAddressU32(), resetMask) != RORC_STATUS_OK) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to arm DDL");
  }
}

void CrorcChannelMaster::startDataReceiving()
{
  auto barAddress = pdaBar.getUserspaceAddress();

  setLoopPerSec(&crorcSharedData.get()->loopPerUsec, &crorcSharedData.get()->pciLoopPerUsec, barAddress); // TODO figure out significance of this call
  ddlFindDiuVersion(barAddress);

  // Preparing the card.
  if (LoopbackMode::EXTERNAL_SIU == getParams().generator.loopbackMode) {
    resetCard(ResetLevel::RORC_DIU_SIU);

    if (rorcCheckLink(barAddress) != RORC_STATUS_OK) {
      ALICEO2_RORC_THROW_EXCEPTION("SIU not seen. Can not clear SIU status");
    } else {
      if (ddlReadSiu(barAddress, 0, DDL_RESPONSE_TIME) == -1) {
        ALICEO2_RORC_THROW_EXCEPTION("SIU read error");
      }
    }
    if (ddlReadDiu(barAddress, 0, DDL_RESPONSE_TIME) == -1) {
      ALICEO2_RORC_THROW_EXCEPTION("DIU read error");
    }
  }

  rorcReset(barAddress, RORC_RESET_FF);

  // Checking if firmware FIFO is empty.
  if (rorcCheckRxFreeFifo(barAddress) != RORC_FF_EMPTY){
    ALICEO2_RORC_THROW_EXCEPTION("Firmware FIFO is not empty");
  }

  auto busAddress = bufferFifo.getScatterGatherList()[0].addressBus;
  rorcStartDataReceiver(barAddress, reinterpret_cast<unsigned long>(busAddress));
}

void CrorcChannelMaster::startDataGenerator(const GeneratorParameters& gen)
{
  auto barAddress = pdaBar.getUserspaceAddress();
  int ret, rounded_len;
  stword_t stw;

  if (LoopbackMode::NONE == gen.loopbackMode) {
    ret = rorcStartTrigger(barAddress, DDL_RESPONSE_TIME * crorcSharedData.get()->pciLoopPerUsec, &stw);

    if (ret == RORC_LINK_NOT_ON) {
      ALICEO2_RORC_THROW_EXCEPTION("Error: LINK IS DOWN, RDYRX command could not be sent");
    }
    if (ret == RORC_STATUS_ERROR) {
      ALICEO2_RORC_THROW_EXCEPTION("Error: RDYRX command could not be sent");
    }
    if (ret == RORC_NOT_ACCEPTED) {
      ALICEO2_RORC_THROW_EXCEPTION("No reply arrived for RDYRX within timeout"); // %d usec\n", DDL_RESPONSE_TIME);
    } else {
      printf("FEE accepted the RDYDX command. Its reply: 0x%08lx\n\n", stw.stw);
    }
  }

  if (rorcArmDataGenerator(barAddress, gen.initialValue, gen.initialWord, gen.pattern, gen.dataSize / 4, gen.seed,
      &rounded_len) != RORC_STATUS_OK) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to arm data generator");
  }

  if (LoopbackMode::INTERNAL_RORC == gen.loopbackMode) {
    rorcParamOn(barAddress, PRORC_PARAM_LOOPB);
    usleep(100000);
  }

  if (LoopbackMode::EXTERNAL_SIU == gen.loopbackMode) {
    ret = ddlSetSiuLoopBack(barAddress, DDL_RESPONSE_TIME * crorcSharedData.get()->pciLoopPerUsec, &stw);
    if (ret != RORC_STATUS_OK) {
      ALICEO2_RORC_THROW_EXCEPTION("SIU loopback error");
    }
    usleep(100000);
    if (rorcCheckLink(barAddress) != RORC_STATUS_OK) {
      ALICEO2_RORC_THROW_EXCEPTION("SIU not seen, can not clear SIU status");
    } else {
      if (ddlReadSiu(barAddress, 0, DDL_RESPONSE_TIME) == -1) {
        ALICEO2_RORC_THROW_EXCEPTION("SIU read error");
      }
    }
    if (ddlReadDiu(barAddress, 0, DDL_RESPONSE_TIME) == -1) {
      ALICEO2_RORC_THROW_EXCEPTION("DIU read error");
    }
  }

  rorcStartDataGenerator(pdaBar.getUserspaceAddress(), gen.maximumEvents);
}

void CrorcChannelMaster::initializeFreeFifo(int pagesToPush)
{
  // Pushing a given number of pages to the firmware FIFO.
  for(int i = 0; i < pagesToPush; ++i){
    pushFreeFifoPage(i, pageAddresses[i].bus);
  }
}

void CrorcChannelMaster::pushFreeFifoPage(int readyFifoIndex, void* pageBusAddress)
{
  size_t pageWords = getParams().dma.pageSize / 4; // Size in 32-bit words
  rorcPushRxFreeFifo(pdaBar.getUserspaceAddress(), reinterpret_cast<uint64_t>(pageBusAddress), pageWords, readyFifoIndex);
}

ChannelMasterInterface::PageHandle CrorcChannelMaster::pushNextPage()
{
  const auto& csd = crorcSharedData.get();

  if (sharedData.get()->dmaState != DmaState::STARTED) {
    ALICEO2_RORC_THROW_EXCEPTION("Not in required DMA state");
  }

  // Handle for next page
  auto handle = PageHandle(csd->fifoIndexWrite);

  // Check if page is available to write to
  if (pageWasReadOut[handle.index] == false) {
    ALICEO2_RORC_THROW_EXCEPTION("Pushing page would overwrite");
  }

  pageWasReadOut[handle.index] = false;
  pushFreeFifoPage(handle.index, pageAddresses[handle.index].bus);

  csd->fifoIndexWrite = (csd->fifoIndexWrite + 1) % getParams().fifo.entries;

  return handle;
}

ChannelMaster::DataArrivalStatus::type CrorcChannelMaster::dataArrived(int index)
{
  auto status = mappedFileFifo.get()->entries[index].status;

  if (status == -1) {
    return DataArrivalStatus::NONE_ARRIVED;
  } else if (status == 0) {
    return DataArrivalStatus::PART_ARRIVED;
  } else {
    return DataArrivalStatus::WHOLE_ARRIVED;
  }
}

bool CrorcChannelMaster::isPageArrived(const PageHandle& handle)
{
  return dataArrived(handle.index) != DataArrivalStatus::NONE_ARRIVED;
}

} // namespace Rorc
} // namespace AliceO2
