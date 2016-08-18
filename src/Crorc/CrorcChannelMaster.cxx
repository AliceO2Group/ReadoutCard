/// \file CrorcChannelMaster.cxx
/// \brief Implementation of the CrorcChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CrorcChannelMaster.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <boost/format.hpp>
#include "c/rorc/rorc.h"
#include "c/rorc/ddl_def.h"
#include "ChannelPaths.h"
#include "RorcStatusCode.h"
#include "Util.h"
#include "ChannelUtilityImpl.h"

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
    << errinfo_rorc_error_message(_err_message) \
    << errinfo_rorc_status_code(_status_code)

/// Amount of additional DMA buffers for this channel
static constexpr int CRORC_BUFFERS_PER_CHANNEL = 1;

/// The index of the DMA buffer of the Ready FIFO
static constexpr int BUFFER_INDEX_FIFO = 1;

CrorcChannelMaster::CrorcChannelMaster(int serial, int channel)
    : ChannelMaster(CARD_TYPE, serial, channel, CRORC_BUFFERS_PER_CHANNEL)
{
  constructorCommon();
}

CrorcChannelMaster::CrorcChannelMaster(int serial, int channel, const ChannelParameters& params)
    : ChannelMaster(CARD_TYPE, serial, channel, params, CRORC_BUFFERS_PER_CHANNEL)
{
  constructorCommon();
}

void CrorcChannelMaster::constructorCommon()
{
  using Util::resetSmartPtr;

  ChannelPaths paths(CARD_TYPE, getSerialNumber(), getChannelNumber());

  resetSmartPtr(mMappedFileFifo, paths.fifo().c_str());

  resetSmartPtr(mBufferReadyFifo, getRorcDevice().getPciDevice(), mMappedFileFifo->getAddress(), mMappedFileFifo->getSize(),
      getBufferId(BUFFER_INDEX_FIFO));

  resetSmartPtr(mCrorcSharedData, paths.state(), getSharedDataSize(), getCrorcSharedDataName().c_str(),
      FileSharedObject::find_or_construct);

  mBufferPageIndexes.resize(READYFIFO_ENTRIES, -1);
  mPageWasReadOut.resize(READYFIFO_ENTRIES, true);

  // Initialize (if needed) the shared data
  const auto& csd = mCrorcSharedData->get();

  if (csd->mInitializationState == InitializationState::INITIALIZED) {
   //cout << "CRORC shared channel state already initialized" << endl;
  }
  else {
   if (csd->mInitializationState == InitializationState::UNKNOWN) {
     //cout << "Warning: unknown CRORC shared channel state. Proceeding with initialization" << endl;
   }
   //cout << "Initializing CRORC shared channel state" << endl;
   csd->initialize();

   //cout << "Clearing readyFifo" << endl;
   mMappedFileFifo->get()->reset();
  }

  auto& params = getParams();

  // Initialize the page addresses
  for (auto& entry : getBufferPages().getScatterGatherList()) {
    if (entry.size < (2l * 1024l * 1024l)) {
      BOOST_THROW_EXCEPTION(CrorcException()
          << errinfo_rorc_error_message("Unsupported configuration: DMA scatter-gather entry size less than 2 MiB")
          << errinfo_rorc_scatter_gather_entry_size(entry.size)
          << errinfo_rorc_possible_causes(
              {"DMA buffer was not allocated in hugepage shared memory (hugetlbfs may not be properly mounted)"}));
    }

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

  if (getPageAddresses().size() <= READYFIFO_ENTRIES) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_rorc_error_message("Insufficient amount of pages fit in DMA buffer"));
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
  // Stopping receiving data
  if (getParams().generator.useDataGenerator) {
    rorcStopDataGenerator(getBarUserspace());
    rorcStopDataReceiver(getBarUserspace());
  } else if(getParams().noRDYRX) {
    // Sending EOBTR to FEE.
    crorcStopTrigger();
  }
}

void CrorcChannelMaster::resetCard(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::Nothing) {
    return;
  }

  auto loopbackMode = getParams().generator.loopbackMode;

  try {
    if (resetLevel == ResetLevel::Rorc) {
      crorcArmDdl(RORC_RESET_RORC);
    }

    if (LoopbackMode::isExternal(loopbackMode)) {
      crorcArmDdl(RORC_RESET_DIU);

      if ((resetLevel == ResetLevel::RorcDiuSiu) && (loopbackMode != LoopbackMode::Diu))
      {
        // Wait a little before SIU reset.

        std::this_thread::sleep_for(std::chrono::microseconds(100000)); /// XXX Why???
        // Reset SIU.
        crorcArmDdl(RORC_RESET_SIU);
        crorcArmDdl(RORC_RESET_DIU);
      }

      crorcArmDdl(RORC_RESET_RORC);
    }
  } catch (RorcException& e) {
    e << errinfo_rorc_reset_level(resetLevel);
    e << errinfo_rorc_loopback_mode(loopbackMode);
    throw;
  }

  // Wait a little after reset.
  std::this_thread::sleep_for(std::chrono::microseconds(100000)); /// XXX Why???
}

void CrorcChannelMaster::startDataGenerator(const GeneratorParameters& gen)
{
  if (LoopbackMode::None == gen.loopbackMode) {
    crorcStartTrigger();
  }

  crorcArmDataGenerator();

  if (LoopbackMode::Rorc == gen.loopbackMode) {
    rorcParamOn(getBarUserspace(), PRORC_PARAM_LOOPB);
    std::this_thread::sleep_for(std::chrono::microseconds(100000)); // XXX Why???
  }

  if (LoopbackMode::Siu == gen.loopbackMode) {
    crorcSetSiuLoopback();
    std::this_thread::sleep_for(std::chrono::microseconds(100000)); // XXX Why???
    crorcCheckLink();
    crorcSiuCommand(0);
    crorcDiuCommand(0);
  }

  rorcStartDataGenerator(getBarUserspace(), gen.maximumEvents);
}

void CrorcChannelMaster::startDataReceiving()
{
  crorcInitDiuVersion();

  // Preparing the card.
  if (LoopbackMode::Siu == getParams().generator.loopbackMode) {
    resetCard(ResetLevel::RorcDiuSiu);
    crorcCheckLink();
    crorcSiuCommand(0);
    crorcDiuCommand(0);
  }

  crorcReset();
  crorcCheckFreeFifoEmpty();
  crorcStartDataReceiver();
}

void CrorcChannelMaster::initializeFreeFifo()
{
  // Pushing a given number of pages to the firmware FIFO.
  for(int i = 0; i < READYFIFO_ENTRIES; ++i){
    getReadyFifo().entries[i].reset();
    pushFreeFifoPage(i, getPageAddresses()[i].bus);
    //while(dataArrived(i) != DataArrivalStatus::WHOLE_ARRIVED) { ; }
  }
}

void CrorcChannelMaster::pushFreeFifoPage(int readyFifoIndex, void* pageBusAddress)
{
  size_t pageWords = getParams().dma.pageSize / 4; // Size in 32-bit words
  rorcPushRxFreeFifo(getBarUserspace(), reinterpret_cast<uint64_t>(pageBusAddress), pageWords, readyFifoIndex);
}

PageHandle CrorcChannelMaster::pushNextPage()
{
  const auto& csd = mCrorcSharedData->get();

  if (getSharedData().dmaState != DmaState::STARTED) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_rorc_error_message("Not in required DMA state")
        << errinfo_rorc_possible_causes({"startDma() not called"}));
  }

  // Handle for next page
  auto fifoIndex = csd->mFifoIndexWrite;
  auto bufferIndex = csd->mBufferPageIndex;

  // Check if page is available to write to
  if (mPageWasReadOut[fifoIndex] == false) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_rorc_error_message("Pushing page would overwrite")
        << errinfo_rorc_fifo_index(fifoIndex));
  }

  mPageWasReadOut[fifoIndex] = false;
  mBufferPageIndexes[fifoIndex] = bufferIndex;

  pushFreeFifoPage(fifoIndex, getPageAddresses()[bufferIndex].bus);

  csd->mFifoIndexWrite = (csd->mFifoIndexWrite + 1) % READYFIFO_ENTRIES;
  csd->mBufferPageIndex = (csd->mBufferPageIndex + 1) % getPageAddresses().size();

  return PageHandle(fifoIndex);
}

void* CrorcChannelMaster::getReadyFifoBusAddress()
{
  return mBufferReadyFifo->getScatterGatherList()[0].addressBus;
}

ReadyFifo& CrorcChannelMaster::getReadyFifo()
{
  return *mMappedFileFifo->get();
}

CrorcChannelMaster::DataArrivalStatus::type CrorcChannelMaster::dataArrived(int index)
{
  auto length = getReadyFifo().entries[index].length;
  auto status = getReadyFifo().entries[index].status;

  if (status == -1) {
    return DataArrivalStatus::NoneArrived;
  } else if (status == 0) {
    return DataArrivalStatus::PartArrived;
  } else if ((status & 0xff) == DTSW) {
    // Note: when internal loopback is used, the length of the event in words is also stored in the status word.
    // For example, the status word could be 0x400082 for events of size 4 kiB
    if ((status & (1 << 31)) != 0) {
      // The error bit is set
      BOOST_THROW_EXCEPTION(CrorcDataArrivalException()
          << errinfo_rorc_error_message("Data arrival status word contains error bits")
          << errinfo_rorc_readyfifo_status(status)
          << errinfo_rorc_readyfifo_length(length)
          << errinfo_rorc_fifo_index(index));
    }
    return DataArrivalStatus::WholeArrived;
  } else {
    BOOST_THROW_EXCEPTION(CrorcDataArrivalException()
        << errinfo_rorc_error_message("Unrecognized data arrival status word")
        << errinfo_rorc_readyfifo_status(status)
        << errinfo_rorc_readyfifo_length(length)
        << errinfo_rorc_fifo_index(index));
  }
}

bool CrorcChannelMaster::isPageArrived(const PageHandle& handle)
{
  return dataArrived(handle.index) == DataArrivalStatus::WholeArrived;
}

Page CrorcChannelMaster::getPage(const PageHandle& handle)
{
  auto fifoIndex = handle.index;
  auto bufferIndex = mBufferPageIndexes[fifoIndex];
  return Page(getPageAddresses()[bufferIndex].user, getReadyFifo().entries[fifoIndex].length);
}

void CrorcChannelMaster::markPageAsRead(const PageHandle& handle)
{
  if (mPageWasReadOut[handle.index]) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_rorc_error_message("Page was already marked as read")
        << errinfo_rorc_page_index(handle.index));
  }

  getReadyFifo().entries[handle.index].reset();
  mPageWasReadOut[handle.index] = true;
}

CardType::type CrorcChannelMaster::getCardType()
{
  return CardType::Crorc;
}

CrorcChannelMaster::CrorcSharedData::CrorcSharedData()
    : mInitializationState(InitializationState::UNKNOWN), mFifoIndexWrite(0), mFifoIndexRead(0), mBufferPageIndex(0), mLoopPerUsec(
        0), mPciLoopPerUsec(0), mRorcRevision(0), mSiuVersion(0), mDiuVersion(0)
{
}

void CrorcChannelMaster::CrorcSharedData::initialize()
{
  mFifoIndexWrite = 0;
  mFifoIndexRead = 0;
  mBufferPageIndex = 0;
  mLoopPerUsec = 0;
  mPciLoopPerUsec = 0;
  mRorcRevision = 0;
  mSiuVersion = 0;
  mDiuVersion = 0;
  mInitializationState = InitializationState::INITIALIZED;
}


void CrorcChannelMaster::crorcArmDataGenerator()
{
  auto& gen = getParams().generator;
  int roundedLen;
  int returnCode = rorcArmDataGenerator(getBarUserspace(), gen.initialValue, gen.initialWord, gen.pattern,
      gen.dataSize / 4, gen.seed, &roundedLen);
  THROW_IF_BAD_STATUS(returnCode, CrorcArmDataGeneratorException()
      ADD_ERRINFO(returnCode, "Failed to arm data generator")
      << errinfo_rorc_generator_pattern(gen.pattern)
      << errinfo_rorc_generator_event_length(gen.dataSize / 4));
}

void CrorcChannelMaster::crorcArmDdl(int resetMask)
{
  auto csd = mCrorcSharedData->get();
  int returnCode = rorcArmDDL(getBarUserspace(), resetMask, csd->mDiuVersion, csd->mPciLoopPerUsec);
  THROW_IF_BAD_STATUS(returnCode, CrorcArmDdlException()
      ADD_ERRINFO(returnCode, "Failed to arm DDL")
      << errinfo_rorc_ddl_reset_mask(b::str(b::format("0x%x") % resetMask)));
}

void CrorcChannelMaster::crorcInitDiuVersion()
{
  auto csd = mCrorcSharedData->get();
  setLoopPerSec(&csd->mLoopPerUsec, &csd->mPciLoopPerUsec, getBarUserspace());
  int returnCode = ddlFindDiuVersion(getBarUserspace(), csd->mPciLoopPerUsec, &csd->mRorcRevision, &csd->mDiuVersion);
  THROW_IF_BAD_STATUS(returnCode, CrorcInitDiuException()
      ADD_ERRINFO(returnCode, "Failed to initialize DIU version"));
}

void CrorcChannelMaster::crorcCheckLink()
{
  int returnCode = rorcCheckLink(getBarUserspace());
  THROW_IF_BAD_STATUS(returnCode, CrorcCheckLinkException()
      ADD_ERRINFO(returnCode, "Bad link status"));
}

void CrorcChannelMaster::crorcSiuCommand(int command)
{
  auto csd = mCrorcSharedData->get();
  int returnCode = ddlReadSiu(getBarUserspace(), command, DDL_RESPONSE_TIME, csd->mPciLoopPerUsec);
  THROW_IF_BAD_STATUS(returnCode, CrorcSiuCommandException()
      ADD_ERRINFO(returnCode, "Failed to send SIU command")
      << errinfo_rorc_siu_command(command));
}

void CrorcChannelMaster::crorcDiuCommand(int command)
{
  auto csd = mCrorcSharedData->get();
  int returnCode = ddlReadDiu(getBarUserspace(), command, DDL_RESPONSE_TIME, csd->mPciLoopPerUsec);
  THROW_IF_BAD_STATUS(returnCode, CrorcSiuCommandException()
      ADD_ERRINFO(returnCode, "Failed to send DIU command")
      << errinfo_rorc_diu_command(command));
}

void CrorcChannelMaster::crorcReset()
{
  auto csd = mCrorcSharedData->get();
  rorcReset(getBarUserspace(), RORC_RESET_FF, csd->mPciLoopPerUsec);
}

void CrorcChannelMaster::crorcCheckFreeFifoEmpty()
{
  int returnCode = rorcCheckRxFreeFifo(getBarUserspace());
  if (returnCode != RORC_FF_EMPTY){
    BOOST_THROW_EXCEPTION(CrorcFreeFifoException()
        ADD_ERRINFO(returnCode, "Free FIFO not empty"));
  }
}

void CrorcChannelMaster::crorcStartDataReceiver()
{
  auto csd = mCrorcSharedData->get();
  auto busAddress = (unsigned long) getReadyFifoBusAddress();
  rorcStartDataReceiver(getBarUserspace(), busAddress, csd->mRorcRevision);
}

void CrorcChannelMaster::crorcSetSiuLoopback()
{
  auto csd = mCrorcSharedData->get();
  stword_t stw;
  int returnCode = ddlSetSiuLoopBack(getBarUserspace(), DDL_RESPONSE_TIME * csd->mPciLoopPerUsec,
      csd->mPciLoopPerUsec, &stw);
  THROW_IF_BAD_STATUS(returnCode, CrorcSiuLoopbackException()
        ADD_ERRINFO(returnCode, "Failed to set SIU loopback"));
}

void CrorcChannelMaster::crorcStartTrigger()
{
  auto csd = mCrorcSharedData->get();
  stword_t stw;
  int returnCode = rorcStartTrigger(getBarUserspace(), DDL_RESPONSE_TIME * csd->mPciLoopPerUsec, &stw);
  THROW_IF_BAD_STATUS(returnCode, CrorcStartTriggerException()
          ADD_ERRINFO(returnCode, "Failed to start trigger"));
}

void CrorcChannelMaster::crorcStopTrigger()
{
  stword_t stw;
  uint64_t timeout = mCrorcSharedData->get()->mPciLoopPerUsec * DDL_RESPONSE_TIME;
  int returnCode = rorcStopTrigger(getBarUserspace(), timeout, &stw);
  THROW_IF_BAD_STATUS(returnCode, CrorcStopTriggerException()
            ADD_ERRINFO(returnCode, "Failed to stop trigger"));
}

std::vector<uint32_t> CrorcChannelMaster::utilityCopyFifo()
{
  std::vector<uint32_t> copy;
  auto& fifo = mMappedFileFifo->get()->dataInt32;
  copy.insert(copy.begin(), fifo.begin(), fifo.end());
  return copy;
}

void CrorcChannelMaster::utilityPrintFifo(std::ostream& os)
{
  ChannelUtility::printCrorcFifo(mMappedFileFifo->get(), os);
}

void CrorcChannelMaster::utilitySetLedState(bool)
{
  BOOST_THROW_EXCEPTION(CrorcException() << errinfo_rorc_error_message("C-RORC does not support setting LED state"));
}

void CrorcChannelMaster::utilitySanityCheck(std::ostream& os)
{
  ChannelUtility::crorcSanityCheck(os, this);
}

void CrorcChannelMaster::utilityCleanupState()
{
  ChannelUtility::crorcCleanupState(ChannelPaths(CARD_TYPE, getSerialNumber(), getChannelNumber()));
}

int CrorcChannelMaster::utilityGetFirmwareVersion()
{
  int pciLoopPerUsec = 0;
  int rorcRevision = 0;
  int diuVersion = 0;
  int returnCode = ddlFindDiuVersion(getBarUserspace(), pciLoopPerUsec, &rorcRevision, &diuVersion);
  THROW_IF_BAD_STATUS(returnCode, CrorcInitDiuException()
      ADD_ERRINFO(returnCode, "Failed to get C-RORC revision"));
  return rorcRevision;
}

} // namespace Rorc
} // namespace AliceO2

