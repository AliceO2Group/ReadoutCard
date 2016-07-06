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

/// Creates a CrorcException and attaches data using the given message string
#define CRORC_EXCEPTION(_err_message) \
  CrorcException() \
      << errinfo_aliceO2_rorc_generic_message(_err_message)

/// Creates a CrorcException and attaches data using the given message string and status code
#define CRORC_STATUS_EXCEPTION(_status_code, _err_message) \
  CRORC_EXCEPTION(_err_message) \
      << errinfo_aliceO2_rorc_status_code(_status_code) \
      << errinfo_aliceO2_rorc_status_string(::AliceO2::Rorc::getRorcStatusString(_status_code))

/// Throws a CrorcException with the given message string
#define THROW_CRORC_EXCEPTION(_err_message) \
  BOOST_THROW_EXCEPTION(CrorcException() \
      << errinfo_aliceO2_rorc_generic_message(_err_message))

/// Throws the given exception if the given status code is not RORC_STATUS_OK
#define THROW_ON_BAD_STATUS(_status_code, _exception) \
  if (_status_code != RORC_STATUS_OK) { \
    BOOST_THROW_EXCEPTION((_exception)); \
  }

/// If the given status code is not RORC_STATUS_OK, creates a CrorcException, attaching data using the given message
/// string and status code, and throws it
#define THROW_STATUS_EXCEPTION_ON_BAD_STATUS(_status_code, _err_message) \
  if (_status_code != RORC_STATUS_OK) { \
    BOOST_THROW_EXCEPTION(CRORC_STATUS_EXCEPTION(_status_code, _err_message)); \
  }

static constexpr int CRORC_BUFFERS_PER_CHANNEL = 2;
static constexpr int BUFFER_INDEX_FIFO = 1;
static const char* CRORC_SHARED_DATA_NAME = "CrorcChannelMasterSharedData";

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
      FileSharedObject::find_or_construct),
  bufferPageIndexes(params.fifo.entries, -1),
  pageWasReadOut(params.fifo.entries, true)
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

   for (int i = 0; i < 128 * 2; ++i) {
     cout << " i " << reinterpret_cast<int32_t*>(mappedFileFifo.getAddress())[i] << endl;
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
    BOOST_THROW_EXCEPTION(AliceO2RorcException()
          << errinfo_aliceO2_rorc_generic_message("Insufficient amount of pages fit in DMA buffer"));
  }
}

CrorcChannelMaster::~CrorcChannelMaster()
{
  cout << "\nDEBUG:\n";
  cout << debug_ss.rdbuf();
}

CrorcChannelMaster::CrorcSharedData::CrorcSharedData()
    : initializationState(InitializationState::UNKNOWN), fifoIndexWrite(0), fifoIndexRead(0), loopPerUsec(
        0), pciLoopPerUsec(0), bufferPageIndex(0), rorcRevision(0), siuVersion(0), diuVersion(0)
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

void CrorcChannelMaster::deviceStartDma()
{
  auto& params = getParams();

  // Initializing the software FIFO
  // initializeReadyFifo(cd); // Already done in constructor

  // Find DIU version, required for armDdl()
  auto csd = crorcSharedData.get();
  ddlFindDiuVersion(pdaBar.getUserspaceAddress(), csd->pciLoopPerUsec, &csd->rorcRevision, &csd->diuVersion);

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
      int returnCode = 0;

      // Clearing SIU/DIU status.
      returnCode = rorcCheckLink(userAddress);
      THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Bad link status");

      returnCode = ddlReadSiu(userAddress, 0, timeout, csd->pciLoopPerUsec);
      THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to clear SIU status");

      returnCode = ddlReadDiu(userAddress, 0, timeout, csd->pciLoopPerUsec);
      THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to clear DIU status");

      // RDYRX command to FEE
      stword_t stw;
      returnCode = rorcStartTrigger(userAddress, respCycle, &stw);
      THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to start trigger");
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
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to stop trigger");

    printf("EOBTR command sent to the FEE\n");

    if (returnCode != RORC_NOT_ACCEPTED) {
      printf("FEE accepted the EOBTR command. Its reply: 0x%08lx\n", stw.stw);
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
  auto csd = crorcSharedData.get();
  int returnCode = rorcArmDDL(pdaBar.getUserspaceAddressU32(), resetMask, csd->diuVersion, csd->pciLoopPerUsec);
  THROW_ON_BAD_STATUS(returnCode, CRORC_STATUS_EXCEPTION(returnCode, "Failed to arm DDL")
      << errinfo_aliceO2_rorc_ddl_reset_mask(b::str(b::format("0x%x") % resetMask)));
}

void CrorcChannelMaster::startDataReceiving()
{
  auto barAddress = pdaBar.getUserspaceAddress();
  auto csd = crorcSharedData.get();

  setLoopPerSec(&crorcSharedData.get()->loopPerUsec, &csd->pciLoopPerUsec, barAddress); // TODO figure out significance of this call
  ddlFindDiuVersion(barAddress, csd->pciLoopPerUsec, &csd->rorcRevision, &csd->diuVersion);

  // Preparing the card.
  if (LoopbackMode::EXTERNAL_SIU == getParams().generator.loopbackMode) {
    resetCard(ResetLevel::RORC_DIU_SIU);
    int returnCode = 0;

    returnCode = rorcCheckLink(barAddress);
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Bad link status");

    returnCode = ddlReadSiu(barAddress, 0, DDL_RESPONSE_TIME, csd->pciLoopPerUsec);
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to read SIU");

    returnCode = ddlReadDiu(barAddress, 0, DDL_RESPONSE_TIME, csd->pciLoopPerUsec);
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to read DIU");
  }

  rorcReset(barAddress, RORC_RESET_FF, csd->pciLoopPerUsec);

  // Checking if firmware FIFO is empty.
  if (rorcCheckRxFreeFifo(barAddress) != RORC_FF_EMPTY){
    THROW_CRORC_EXCEPTION("Firmware FIFO is not empty");
  }

  auto busAddress = bufferFifo.getScatterGatherList()[0].addressBus;
  rorcStartDataReceiver(barAddress, reinterpret_cast<unsigned long>(busAddress), csd->rorcRevision);
}

void CrorcChannelMaster::startDataGenerator(const GeneratorParameters& gen)
{
  auto barAddress = pdaBar.getUserspaceAddress();
  int returnCode, rounded_len;
  stword_t stw;
  auto csd = crorcSharedData.get();

  if (LoopbackMode::NONE == gen.loopbackMode) {
    returnCode = rorcStartTrigger(barAddress, DDL_RESPONSE_TIME * csd->pciLoopPerUsec, &stw);
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to start trigger");
  }

  returnCode = rorcArmDataGenerator(barAddress, gen.initialValue, gen.initialWord, gen.pattern, gen.dataSize / 4, gen.seed,
      &rounded_len);
  THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to arm data generator");

  if (LoopbackMode::INTERNAL_RORC == gen.loopbackMode) {
    rorcParamOn(barAddress, PRORC_PARAM_LOOPB);
    usleep(100000); // XXX Why???
  }

  if (LoopbackMode::EXTERNAL_SIU == gen.loopbackMode) {
    returnCode = ddlSetSiuLoopBack(barAddress, DDL_RESPONSE_TIME * csd->pciLoopPerUsec, csd->pciLoopPerUsec, &stw);
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to set SIU loopback");

    usleep(100000); // XXX Why???

    returnCode = rorcCheckLink(barAddress);
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Bad link status");

    returnCode = ddlReadSiu(barAddress, 0, DDL_RESPONSE_TIME, csd->pciLoopPerUsec);
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to read SIU");

    returnCode = ddlReadDiu(barAddress, 0, DDL_RESPONSE_TIME, csd->pciLoopPerUsec);
    THROW_STATUS_EXCEPTION_ON_BAD_STATUS(returnCode, "Failed to read DIU");
  }

  rorcStartDataGenerator(pdaBar.getUserspaceAddress(), gen.maximumEvents);
}

void CrorcChannelMaster::initializeFreeFifo(int pagesToPush)
{
  // Pushing a given number of pages to the firmware FIFO.
  for(int i = 0; i < pagesToPush; ++i){
    mappedFileFifo.get()->entries[i].reset();
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
    THROW_CRORC_EXCEPTION("Not in required DMA state");
  }

  // Handle for next page
  auto fifoIndex = csd->fifoIndexWrite;
  auto bufferIndex = csd->bufferPageIndex;

  // Check if page is available to write to
  if (pageWasReadOut[fifoIndex] == false) {
    BOOST_THROW_EXCEPTION(CRORC_EXCEPTION("Pushing page would overwrite")
        << errinfo_aliceO2_rorc_fifo_index(fifoIndex));
  }

  pageWasReadOut[fifoIndex] = false;
  bufferPageIndexes[fifoIndex] = bufferIndex;

  pushFreeFifoPage(fifoIndex, pageAddresses[bufferIndex].bus);

  csd->fifoIndexWrite = (csd->fifoIndexWrite + 1) % getParams().fifo.entries;
  csd->bufferPageIndex = (csd->bufferPageIndex + 1) % pageAddresses.size();

  return PageHandle(fifoIndex);
}

CrorcChannelMaster::DataArrivalStatus::type CrorcChannelMaster::dataArrived(int index)
{
  //auto addr = reinterpret_cast<int32_t*>(mappedFileFifo.getAddress());
  //auto status = addr[(index * 2) + 1];

  auto length = mappedFileFifo.get()->entries[index].length;
  auto status = mappedFileFifo.get()->entries[index].status;

  debug_ss << std::setw(6) << index << std::setw(12) << std::hex << status << std::dec << '\n';

  if (status == -1) {
    return DataArrivalStatus::NONE_ARRIVED;
  } else if (status == 0) {
    return DataArrivalStatus::PART_ARRIVED;
  } else if ((status & 0xff) == DTSW) {
    // Note: when internal loopback is used, the length of the event in words is also stored in the status word.
    // For example, the status word could be 0x400082 for events of size 4 kiB
    if ((status & (1 << 32)) != 0) {
      // The error bit is set
      BOOST_THROW_EXCEPTION(CRORC_EXCEPTION("Data arrival status word contains error bits")
          << errinfo_aliceO2_rorc_readyfifo_status(b::str(b::format("0x%x") % status))
          << errinfo_aliceO2_rorc_readyfifo_length(length)
          << errinfo_aliceO2_rorc_fifo_index(index));
    }
    return DataArrivalStatus::WHOLE_ARRIVED;
  } else {
    BOOST_THROW_EXCEPTION(CRORC_EXCEPTION("Unrecognized data arrival status word")
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
  return Page(pageAddresses[bufferIndex].user, mappedFileFifo.get()->entries[fifoIndex].length);
}

void CrorcChannelMaster::markPageAsRead(const PageHandle& handle)
{
  if (pageWasReadOut[handle.index]) {
    BOOST_THROW_EXCEPTION(CRORC_EXCEPTION("Page was already marked as read")
        << errinfo_aliceO2_rorc_page_index(handle.index));
  }

  mappedFileFifo.get()->entries[handle.index].reset();
  pageWasReadOut[handle.index] = true;
}


} // namespace Rorc
} // namespace AliceO2
