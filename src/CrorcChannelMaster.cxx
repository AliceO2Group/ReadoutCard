///
/// \file CrorcChannelMaster.cxx
/// \author Pascal Boeschoten
///

#include "CrorcChannelMaster.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <boost/format.hpp>
#include "c/rorc/rorc.h"
#include "ChannelPaths.h"
#include "RorcStatusCode.h"

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

/// Throws the given exception if the given status code is not equal to RORC_STATUS_OK
#define THROW_IF_BAD_STATUS(_status_code, _exception) \
  if (_status_code != RORC_STATUS_OK) { \
    BOOST_THROW_EXCEPTION((_exception)); \
  }

/// Adds errinfo using the given status code and error message
#define ADD_ERRINFO(_status_code, _err_message) \
    << errinfo_aliceO2_rorc_generic_message(_err_message) \
    << errinfo_aliceO2_rorc_status_code(_status_code) \
    << errinfo_aliceO2_rorc_status_code_string(::AliceO2::Rorc::getRorcStatusString(_status_code))

/// Adds errinfo using the given error message
#define ADD_ERRINFO_MSG(_err_message) \
      << errinfo_aliceO2_rorc_generic_message(_err_message) \

/// Amount of DMA buffers for this channel
static constexpr int CRORC_BUFFERS_PER_CHANNEL = 2;

/// The index of the DMA buffer of the Ready FIFO
static constexpr int BUFFER_INDEX_FIFO = 1;

CrorcChannelMaster::CrorcChannelMaster(int serial, int channel, const ChannelParameters& params)
: ChannelMaster(serial, channel, params, CRORC_BUFFERS_PER_CHANNEL),
  mappedFileFifo(
      ChannelPaths::fifo(serial, channel).c_str()),
  bufferReadyFifo(
      pdaDevice.getPciDevice(),
      mappedFileFifo.getAddress(),
      mappedFileFifo.getSize(),
      getBufferId(BUFFER_INDEX_FIFO)),
  crorcSharedData(
      ChannelPaths::state(serial, channel),
      sharedDataSize(),
      crorcSharedDataName(),
      FileSharedObject::find_or_construct),
  bufferPageIndexes(ReadyFifo::FIFO_ENTRIES, -1),
  pageWasReadOut(ReadyFifo::FIFO_ENTRIES, true)
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
   csd->initialize();

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
     pa.bus = (void*) (((char*) entry.addressBus) + offset);
     pa.user = (void*) (((char*) entry.addressUser) + offset);
     pageAddresses.push_back(pa);
   }
  }

  if (pageAddresses.size() <= ReadyFifo::FIFO_ENTRIES) {
    BOOST_THROW_EXCEPTION(AliceO2RorcException()
          << errinfo_aliceO2_rorc_generic_message("Insufficient amount of pages fit in DMA buffer"));
  }
}

CrorcChannelMaster::~CrorcChannelMaster()
{
}

void CrorcChannelMaster::deviceStartDma()
{
  auto& params = getParams();

  // Find DIU version, required for armDdl()
  crorcInitDiuVersion();

  // Resetting the card,according to the RESET LEVEL parameter
  resetCard(params.initialResetLevel);

  // Setting the card to be able to receive data
  startDataReceiving();

  // Initializing the firmware FIFO, pushing (entries) pages
  initializeFreeFifo();

  if (params.generator.useDataGenerator) {
    // Starting the data generator
    startDataGenerator(params.generator);
  } else {
    if (!params.noRDYRX) {
      // Clearing SIU/DIU status.
      crorcCheckLink();
      crorcSiuCommand(0);
      crorcDiuCommand(0);

      // RDYRX command to FEE
      crorcStartTrigger();
    }
  }
}

int rorcStopDataGenerator(volatile uint32_t* buff)
{
  rorcWriteReg(buff, C_CSR, DRORC_CMD_STOP_DG);
  return RORC_STATUS_OK;
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
    crorcStopTrigger();
  }
}

void CrorcChannelMaster::resetCard(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::NOTHING) {
    return;
  }

  auto loopbackMode = getParams().generator.loopbackMode;

  try {
    if (resetLevel == ResetLevel::RORC_ONLY) {
      crorcArmDdl(RORC_RESET_RORC);
    }

    if (LoopbackMode::isExternal(loopbackMode)) {
      crorcArmDdl(RORC_RESET_DIU);

      if ((resetLevel == ResetLevel::RORC_DIU_SIU) && (loopbackMode != LoopbackMode::EXTERNAL_DIU))
      {
        // Wait a little before SIU reset.
        usleep(100000); /// XXX Dirty...
        // Reset SIU.
        crorcArmDdl(RORC_RESET_SIU);
        crorcArmDdl(RORC_RESET_DIU);
      }

      crorcArmDdl(RORC_RESET_RORC);
    }
  } catch (AliceO2RorcException& e) {
    e << errinfo_aliceO2_rorc_reset_level(resetLevel);
    e << errinfo_aliceO2_rorc_reset_level_string(ResetLevel::toString(resetLevel));
    e << errinfo_aliceO2_rorc_loopback_mode(loopbackMode);
    e << errinfo_aliceO2_rorc_loopback_mode_string(LoopbackMode::toString(loopbackMode));
    throw;
  }

  // Wait a little after reset.
  usleep(100000); /// XXX Dirty...
}

void CrorcChannelMaster::startDataGenerator(const GeneratorParameters& gen)
{
  if (LoopbackMode::NONE == gen.loopbackMode) {
    crorcStartTrigger();
  }

  crorcArmDataGenerator();

  if (LoopbackMode::INTERNAL_RORC == gen.loopbackMode) {
    rorcParamOn(pdaBar.getUserspaceAddress(), PRORC_PARAM_LOOPB);
    usleep(100000); // XXX Why???
  }

  if (LoopbackMode::EXTERNAL_SIU == gen.loopbackMode) {
    crorcSetSiuLoopback();
    usleep(100000); // XXX Why???
    crorcCheckLink();
    crorcSiuCommand(0);
    crorcDiuCommand(0);
  }

  rorcStartDataGenerator(pdaBar.getUserspaceAddress(), gen.maximumEvents);
}

void CrorcChannelMaster::initializeFreeFifo()
{
  // Pushing a given number of pages to the firmware FIFO.
  for(int i = 0; i < ReadyFifo::FIFO_ENTRIES; ++i){
    getReadyFifo().entries[i].reset();
    pushFreeFifoPage(i, pageAddresses[i].bus);
    //while(dataArrived(i) != DataArrivalStatus::WHOLE_ARRIVED) { ; }
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
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_aliceO2_rorc_generic_message("Not in required DMA state"));
  }

  // Handle for next page
  auto fifoIndex = csd->fifoIndexWrite;
  auto bufferIndex = csd->bufferPageIndex;

  // Check if page is available to write to
  if (pageWasReadOut[fifoIndex] == false) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_aliceO2_rorc_generic_message("Pushing page would overwrite")
        << errinfo_aliceO2_rorc_fifo_index(fifoIndex));
  }

  pageWasReadOut[fifoIndex] = false;
  bufferPageIndexes[fifoIndex] = bufferIndex;

  pushFreeFifoPage(fifoIndex, pageAddresses[bufferIndex].bus);

  csd->fifoIndexWrite = (csd->fifoIndexWrite + 1) % ReadyFifo::FIFO_ENTRIES;
  csd->bufferPageIndex = (csd->bufferPageIndex + 1) % pageAddresses.size();

  return PageHandle(fifoIndex);
}

void* CrorcChannelMaster::getReadyFifoBusAddress()
{
  return bufferReadyFifo.getScatterGatherList()[0].addressBus;
}

ReadyFifo& CrorcChannelMaster::getReadyFifo()
{
  return *mappedFileFifo.get();
}

CrorcChannelMaster::DataArrivalStatus::type CrorcChannelMaster::dataArrived(int index)
{
  auto length = getReadyFifo().entries[index].length;
  auto status = getReadyFifo().entries[index].status;

  if (status == -1) {
    return DataArrivalStatus::NONE_ARRIVED;
  } else if (status == 0) {
    return DataArrivalStatus::PART_ARRIVED;
  } else if ((status & 0xff) == DTSW) {
    // Note: when internal loopback is used, the length of the event in words is also stored in the status word.
    // For example, the status word could be 0x400082 for events of size 4 kiB
    if ((status & (1 << 31)) != 0) {
      // The error bit is set
      BOOST_THROW_EXCEPTION(CrorcDataArrivalException()
          << errinfo_aliceO2_rorc_generic_message("Data arrival status word contains error bits")
          << errinfo_aliceO2_rorc_readyfifo_status(b::str(b::format("0x%x") % status))
          << errinfo_aliceO2_rorc_readyfifo_length(length)
          << errinfo_aliceO2_rorc_fifo_index(index));
    }
    return DataArrivalStatus::WHOLE_ARRIVED;
  } else {
    BOOST_THROW_EXCEPTION(CrorcDataArrivalException()
        << errinfo_aliceO2_rorc_generic_message("Unrecognized data arrival status word")
        << errinfo_aliceO2_rorc_readyfifo_status(b::str(b::format("0x%x") % status))
        << errinfo_aliceO2_rorc_readyfifo_length(length)
        << errinfo_aliceO2_rorc_fifo_index(index));
  }
}

bool CrorcChannelMaster::isPageArrived(const PageHandle& handle)
{
  return dataArrived(handle.index) == DataArrivalStatus::WHOLE_ARRIVED;
}

Page CrorcChannelMaster::getPage(const PageHandle& handle)
{
  auto fifoIndex = handle.index;
  auto bufferIndex = bufferPageIndexes[fifoIndex];
  return Page(pageAddresses[bufferIndex].user, getReadyFifo().entries[fifoIndex].length);
}

void CrorcChannelMaster::markPageAsRead(const PageHandle& handle)
{
  if (pageWasReadOut[handle.index]) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_aliceO2_rorc_generic_message("Page was already marked as read")
        << errinfo_aliceO2_rorc_page_index(handle.index));
  }

  getReadyFifo().entries[handle.index].reset();
  pageWasReadOut[handle.index] = true;
}

CrorcChannelMaster::CrorcSharedData::CrorcSharedData()
    : initializationState(InitializationState::UNKNOWN), fifoIndexWrite(0), fifoIndexRead(0), bufferPageIndex(0), loopPerUsec(
        0), pciLoopPerUsec(0), rorcRevision(0), siuVersion(0), diuVersion(0)
{
}

void CrorcChannelMaster::CrorcSharedData::initialize()
{
  fifoIndexWrite = 0;
  fifoIndexRead = 0;
  bufferPageIndex = 0;
  loopPerUsec = 0;
  pciLoopPerUsec = 0;
  rorcRevision = 0;
  siuVersion = 0;
  diuVersion = 0;
  initializationState = InitializationState::INITIALIZED;
}


void CrorcChannelMaster::crorcArmDataGenerator()
{
  auto& gen = getParams().generator;
  int roundedLen;
  int returnCode = rorcArmDataGenerator(pdaBar.getUserspaceAddress(), gen.initialValue, gen.initialWord, gen.pattern,
      gen.dataSize / 4, gen.seed, &roundedLen);

  THROW_IF_BAD_STATUS(returnCode, CrorcArmDataGeneratorException()
      ADD_ERRINFO(returnCode, "Failed to arm data generator")
      << errinfo_aliceO2_rorc_generator_pattern(gen.pattern)
      << errinfo_aliceO2_rorc_generator_event_length(gen.dataSize / 4));
}

void CrorcChannelMaster::crorcArmDdl(int resetMask)
{
  auto csd = crorcSharedData.get();
  int returnCode = rorcArmDDL(pdaBar.getUserspaceAddressU32(), resetMask, csd->diuVersion, csd->pciLoopPerUsec);

  THROW_IF_BAD_STATUS(returnCode, CrorcArmDdlException()
      ADD_ERRINFO(returnCode, "Failed to arm DDL")
      << errinfo_aliceO2_rorc_ddl_reset_mask(b::str(b::format("0x%x") % resetMask)));
}

void CrorcChannelMaster::crorcInitDiuVersion()
{
  auto barAddress = pdaBar.getUserspaceAddress();
  auto csd = crorcSharedData.get();
  setLoopPerSec(&crorcSharedData.get()->loopPerUsec, &csd->pciLoopPerUsec, barAddress);
  int returnCode = ddlFindDiuVersion(barAddress, csd->pciLoopPerUsec, &csd->rorcRevision, &csd->diuVersion);

  THROW_IF_BAD_STATUS(returnCode, CrorcInitDiuException()
      ADD_ERRINFO(returnCode, "Failed to initialize DIU version"));
}

void CrorcChannelMaster::crorcCheckLink()
{
  int returnCode = rorcCheckLink(pdaBar.getUserspaceAddress());

  THROW_IF_BAD_STATUS(returnCode, CrorcCheckLinkException()
      ADD_ERRINFO(returnCode, "Bad link status"));
}

void CrorcChannelMaster::crorcSiuCommand(int command)
{
  auto csd = crorcSharedData.get();
  int returnCode = ddlReadSiu(pdaBar.getUserspaceAddress(), command, DDL_RESPONSE_TIME, csd->pciLoopPerUsec);

  THROW_IF_BAD_STATUS(returnCode, CrorcSiuCommandException()
      ADD_ERRINFO(returnCode, "Failed to send SIU command")
      << errinfo_aliceO2_rorc_siu_command(command));
}

void CrorcChannelMaster::crorcDiuCommand(int command)
{
  auto csd = crorcSharedData.get();
  int returnCode = ddlReadDiu(pdaBar.getUserspaceAddress(), command, DDL_RESPONSE_TIME, csd->pciLoopPerUsec);

  THROW_IF_BAD_STATUS(returnCode, CrorcSiuCommandException()
      ADD_ERRINFO(returnCode, "Failed to send DIU command")
      << errinfo_aliceO2_rorc_diu_command(command));
}

void CrorcChannelMaster::crorcReset()
{
  auto csd = crorcSharedData.get();
  rorcReset(pdaBar.getUserspaceAddress(), RORC_RESET_FF, csd->pciLoopPerUsec);
}

void CrorcChannelMaster::crorcCheckFreeFifoEmpty()
{
  int returnCode = rorcCheckRxFreeFifo(pdaBar.getUserspaceAddress());
  if (returnCode != RORC_FF_EMPTY){
    BOOST_THROW_EXCEPTION(CrorcFreeFifoException()
        ADD_ERRINFO(returnCode, "Free FIFO not empty"));
  }
}

void CrorcChannelMaster::crorcStartDataReceiver()
{
  auto csd = crorcSharedData.get();
  auto busAddress = (unsigned long) getReadyFifoBusAddress();
  rorcStartDataReceiver(pdaBar.getUserspaceAddress(), busAddress, csd->rorcRevision);
}

void CrorcChannelMaster::crorcSetSiuLoopback()
{
  auto csd = crorcSharedData.get();
  stword_t stw;
  int returnCode = ddlSetSiuLoopBack(pdaBar.getUserspaceAddress(), DDL_RESPONSE_TIME * csd->pciLoopPerUsec,
      csd->pciLoopPerUsec, &stw);

  THROW_IF_BAD_STATUS(returnCode, CrorcSiuLoopbackException()
        ADD_ERRINFO(returnCode, "Failed to set SIU loopback"));
}

void CrorcChannelMaster::crorcStartTrigger()
{
  auto csd = crorcSharedData.get();
  stword_t stw;
  int returnCode = rorcStartTrigger(pdaBar.getUserspaceAddress(), DDL_RESPONSE_TIME * csd->pciLoopPerUsec, &stw);

  THROW_IF_BAD_STATUS(returnCode, CrorcStartTriggerException()
          ADD_ERRINFO(returnCode, "Failed to start trigger"));
}

void CrorcChannelMaster::crorcStopTrigger()
{
  stword_t stw;
  uint64_t timeout = crorcSharedData.get()->pciLoopPerUsec * DDL_RESPONSE_TIME;
  int returnCode = rorcStopTrigger(pdaBar.getUserspaceAddress(), timeout, &stw);

  THROW_IF_BAD_STATUS(returnCode, CrorcStopTriggerException()
            ADD_ERRINFO(returnCode, "Failed to stop trigger"));
}

void CrorcChannelMaster::startDataReceiving()
{
  crorcInitDiuVersion();

  // Preparing the card.
  if (LoopbackMode::EXTERNAL_SIU == getParams().generator.loopbackMode) {
    resetCard(ResetLevel::RORC_DIU_SIU);
    crorcCheckLink();
    crorcSiuCommand(0);
    crorcDiuCommand(0);
  }

  crorcReset();
  crorcCheckFreeFifoEmpty();
  crorcStartDataReceiver();
}

} // namespace Rorc
} // namespace AliceO2
