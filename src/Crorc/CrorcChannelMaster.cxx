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
#include "ChannelUtilityImpl.h"
#include "RorcStatusCode.h"
#include "Util.h"

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
using std::cout;
using std::endl;
using namespace std::literals;

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

CrorcChannelMaster::CrorcChannelMaster(const Parameters& parameters)
    : ChannelMasterBase(CARD_TYPE, parameters, allowedChannels(), sizeof(ReadyFifo))
{
  using Util::resetSmartPtr;
  auto& params = getChannelParameters();

  ChannelPaths paths(CARD_TYPE, getSerialNumber(), getChannelNumber());

  getFifoUser()->reset();

  mBufferPageIndexes.resize(READYFIFO_ENTRIES, -1);
  mPageWasReadOut.resize(READYFIFO_ENTRIES, true);

  if (getPageAddresses().size() <= READYFIFO_ENTRIES) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << errinfo_rorc_error_message("Insufficient amount of pages fit in DMA buffer")
        << errinfo_rorc_pages(getPageAddresses().size())
        << errinfo_rorc_dma_buffer_size(params.dma.bufferSize)
        << errinfo_rorc_dma_page_size(params.dma.pageSize));
  }

  mPageManager.setAmountOfPages(getPageAddresses().size());
}

auto CrorcChannelMaster::allowedChannels() -> AllowedChannels {
  return {0, 1, 2, 3, 4, 5};
}

CrorcChannelMaster::~CrorcChannelMaster()
{
}

void CrorcChannelMaster::deviceStartDma()
{
  auto& params = getChannelParameters();

  // Find DIU version, required for armDdl()
  crorcInitDiuVersion();

  // Resetting the card,according to the RESET LEVEL parameter
  deviceResetChannel(params.initialResetLevel);

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
      crorcSiuCommand(RandCIFST);
      crorcDiuCommand(RandCIFST);

      // RDYRX command to FEE
      crorcStartTrigger();
    }
  }

  std::this_thread::sleep_for(10ms);
  getFifoUser()->reset();
}

int rorcStopDataGenerator(volatile uint32_t* buff)
{
  rorcWriteReg(buff, C_CSR, DRORC_CMD_STOP_DG);
  return RORC_STATUS_OK;
}

void CrorcChannelMaster::deviceStopDma()
{
  // Stopping receiving data
  if (getChannelParameters().generator.useDataGenerator) {
    rorcStopDataGenerator(getBarUserspace());
    rorcStopDataReceiver(getBarUserspace());
  } else if (getChannelParameters().noRDYRX) {
    // Sending EOBTR to FEE.
    crorcStopTrigger();
  }
}

void CrorcChannelMaster::deviceResetChannel(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::Nothing) {
    return;
  }

  auto loopbackMode = getChannelParameters().generator.loopbackMode;

  try {
    if (resetLevel == ResetLevel::Rorc) {
      crorcReset(RORC_RESET_FF);
      crorcReset(RORC_RESET_RORC);
    }

    if (LoopbackMode::isExternal(loopbackMode)) {
      crorcArmDdl(RORC_RESET_DIU);

      if ((resetLevel == ResetLevel::RorcDiuSiu) && (loopbackMode != LoopbackMode::Diu))
      {
        // Wait a little before SIU reset.
        std::this_thread::sleep_for(100ms); /// XXX Why???
        // Reset SIU.
        crorcArmDdl(RORC_RESET_SIU);
        crorcArmDdl(RORC_RESET_DIU);
      }

      crorcArmDdl(RORC_RESET_RORC);
    }
  }
  catch (Exception& e) {
    e << errinfo_rorc_reset_level(resetLevel);
    e << errinfo_rorc_loopback_mode(loopbackMode);
    throw;
  }

  // Wait a little after reset.
  std::this_thread::sleep_for(100ms); /// XXX Why???
}

void CrorcChannelMaster::startDataGenerator(const GeneratorParameters& gen)
{
  if (LoopbackMode::None == gen.loopbackMode) {
    crorcStartTrigger();
  }

  crorcArmDataGenerator();

  if (LoopbackMode::Rorc == gen.loopbackMode) {
    rorcParamOn(getBarUserspace(), PRORC_PARAM_LOOPB);
    std::this_thread::sleep_for(100ms); // XXX Why???
  }

  if (LoopbackMode::Siu == gen.loopbackMode) {
    crorcSetSiuLoopback();
    std::this_thread::sleep_for(100ms); // XXX Why???
    crorcCheckLink();
    crorcSiuCommand(RandCIFST);
    crorcDiuCommand(RandCIFST);
  }

  rorcStartDataGenerator(getBarUserspace(), gen.maximumEvents);
}

void CrorcChannelMaster::startDataReceiving()
{
  crorcInitDiuVersion();

  // Preparing the card.
  if (LoopbackMode::Siu == getChannelParameters().generator.loopbackMode) {
    deviceResetChannel(ResetLevel::RorcDiuSiu);
    crorcCheckLink();
    crorcSiuCommand(RandCIFST);
    crorcDiuCommand(RandCIFST);
  }

  crorcReset(RORC_RESET_FF);
  crorcCheckFreeFifoEmpty();
  crorcStartDataReceiver();
}

void CrorcChannelMaster::initializeFreeFifo()
{
  // Pushing a given number of pages to the firmware FIFO.
  for(int i = 0; i < READYFIFO_ENTRIES; ++i){
    getFifoUser()->entries[i].reset();
    pushFreeFifoPage(i, getPageAddresses()[i].bus);
  }
}

void CrorcChannelMaster::pushFreeFifoPage(int readyFifoIndex, volatile void* pageBusAddress)
{
  size_t pageWords = getChannelParameters().dma.pageSize / 4; // Size in 32-bit words
  rorcPushRxFreeFifo(getBarUserspace(), reinterpret_cast<uint64_t>(pageBusAddress), pageWords, readyFifoIndex);
}

//PageHandle CrorcChannelMaster::pushNextPage()
//{
//  const auto& csd = mCrorcSharedData->get();
//
//  if (getSharedData().mDmaState != DmaState::STARTED) {
//    BOOST_THROW_EXCEPTION(CrorcException()
//        << errinfo_rorc_error_message("Not in required DMA state")
//        << errinfo_rorc_possible_causes({"startDma() not called"}));
//  }
//
//  // Handle for next page
//  auto fifoIndex = csd->mFifoIndexWrite;
//  auto bufferIndex = csd->mBufferPageIndex;
//
//  // Check if page is available to write to
//  if (mPageWasReadOut[fifoIndex] == false) {
//    BOOST_THROW_EXCEPTION(CrorcException()
//        << errinfo_rorc_error_message("Pushing page would overwrite")
//        << errinfo_rorc_fifo_index(fifoIndex));
//  }
//
//  mPageWasReadOut[fifoIndex] = false;
//  mBufferPageIndexes[fifoIndex] = bufferIndex;
//
//  pushFreeFifoPage(fifoIndex, getPageAddresses()[bufferIndex].bus);
//
//  csd->mFifoIndexWrite = (csd->mFifoIndexWrite + 1) % READYFIFO_ENTRIES;
//  csd->mBufferPageIndex = (csd->mBufferPageIndex + 1) % getPageAddresses().size();
//
//  return PageHandle(fifoIndex);
//}
//
//bool CrorcChannelMaster::isPageArrived(const PageHandle& handle)
//{
//  return dataArrived(handle.index) == DataArrivalStatus::WholeArrived;
//}
//
//Page CrorcChannelMaster::getPage(const PageHandle& handle)
//{
//  auto fifoIndex = handle.index;
//  auto bufferIndex = mBufferPageIndexes[fifoIndex];
//  return Page(getPageAddresses()[bufferIndex].user, getReadyFifo().entries[fifoIndex].length);
//}
//
//void CrorcChannelMaster::markPageAsRead(const PageHandle& handle)
//{
//  if (mPageWasReadOut[handle.index]) {
//    BOOST_THROW_EXCEPTION(CrorcException()
//        << errinfo_rorc_error_message("Page was already marked as read")
//        << errinfo_rorc_page_index(handle.index));
//  }
//
//  getReadyFifo().entries[handle.index].reset();
//  mPageWasReadOut[handle.index] = true;
//}

CrorcChannelMaster::DataArrivalStatus::type CrorcChannelMaster::dataArrived(int index)
{
  auto length = getFifoUser()->entries[index].length;
  auto status = getFifoUser()->entries[index].status;

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
  }

  BOOST_THROW_EXCEPTION(CrorcDataArrivalException()
      << errinfo_rorc_error_message("Unrecognized data arrival status word")
      << errinfo_rorc_readyfifo_status(status)
      << errinfo_rorc_readyfifo_length(length)
      << errinfo_rorc_fifo_index(index));
}

CardType::type CrorcChannelMaster::getCardType()
{
  return CardType::Crorc;
}

int CrorcChannelMaster::fillFifo(int maxFill)
{
  CHANNELMASTER_LOCKGUARD();

  auto isArrived = [&](int descriptorIndex) {
    return dataArrived(descriptorIndex) == DataArrivalStatus::WholeArrived;
  };

  auto resetDescriptor = [&](int descriptorIndex) {
    getFifoUser()->entries[descriptorIndex].reset();
  };

  auto push = [&](int bufferIndex, int descriptorIndex) {
    pushFreeFifoPage(descriptorIndex, getPageAddresses()[bufferIndex].bus);
  };

  mPageManager.handleArrivals(isArrived, resetDescriptor);
  int pushCount = mPageManager.pushPages(maxFill, push);
  return pushCount;
}

int CrorcChannelMaster::getAvailableCount()
{
  CHANNELMASTER_LOCKGUARD();

  return mPageManager.getArrivedCount();
}

auto CrorcChannelMaster::popPageInternal(const MasterSharedPtr& channel) -> std::shared_ptr<Page>
{
  CHANNELMASTER_LOCKGUARD();

  if (auto page = mPageManager.useArrivedPage()) {
    int bufferIndex = *page;
    return std::make_shared<Page>(getPageAddresses()[bufferIndex].user, getChannelParameters().dma.pageSize,
        bufferIndex, channel);
  }
  return nullptr;
}

void CrorcChannelMaster::freePage(const Page& page)
{
  CHANNELMASTER_LOCKGUARD();

  mPageManager.freePage(page.getId());
}

void CrorcChannelMaster::crorcArmDataGenerator()
{
  auto& gen = getChannelParameters().generator;
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
  int returnCode = rorcArmDDL(getBarUserspace(), resetMask, mDiuVersion, mPciLoopPerUsec);
  THROW_IF_BAD_STATUS(returnCode, CrorcArmDdlException()
      ADD_ERRINFO(returnCode, "Failed to arm DDL")
      << errinfo_rorc_ddl_reset_mask(b::str(b::format("0x%x") % resetMask)));
}

void CrorcChannelMaster::crorcInitDiuVersion()
{
  setLoopPerSec(&mLoopPerUsec, &mPciLoopPerUsec, getBarUserspace());
  int returnCode = ddlFindDiuVersion(getBarUserspace(), mPciLoopPerUsec, &mRorcRevision, &mDiuVersion);
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
  int returnCode = ddlReadSiu(getBarUserspace(), command, DDL_RESPONSE_TIME, mPciLoopPerUsec);
  THROW_IF_BAD_STATUS(returnCode, CrorcSiuCommandException()
      ADD_ERRINFO(returnCode, "Failed to send SIU command")
      << errinfo_rorc_siu_command(command));
}

void CrorcChannelMaster::crorcDiuCommand(int command)
{
  int returnCode = ddlReadDiu(getBarUserspace(), command, DDL_RESPONSE_TIME, mPciLoopPerUsec);
  THROW_IF_BAD_STATUS(returnCode, CrorcSiuCommandException()
      ADD_ERRINFO(returnCode, "Failed to send DIU command")
      << errinfo_rorc_diu_command(command));
}

void CrorcChannelMaster::crorcReset(int command)
{
  rorcReset(getBarUserspace(), command, mPciLoopPerUsec);
}

void CrorcChannelMaster::crorcCheckFreeFifoEmpty()
{
  int returnCode = rorcCheckRxFreeFifo(getBarUserspace());
  if (returnCode != RORC_FF_EMPTY){
    BOOST_THROW_EXCEPTION(CrorcFreeFifoException()
        ADD_ERRINFO(returnCode, "Free FIFO not empty")
        << errinfo_rorc_possible_causes({"Previous DMA did not get/free all received pages"}));
  }
}

void CrorcChannelMaster::crorcStartDataReceiver()
{
  auto busAddress = reinterpret_cast<unsigned long>(getFifoBus());
  rorcStartDataReceiver(getBarUserspace(), busAddress, mRorcRevision);
}

void CrorcChannelMaster::crorcSetSiuLoopback()
{
  stword_t stw;
  int returnCode = ddlSetSiuLoopBack(getBarUserspace(), DDL_RESPONSE_TIME * mPciLoopPerUsec, mPciLoopPerUsec, &stw);
  THROW_IF_BAD_STATUS(returnCode, CrorcSiuLoopbackException()
        ADD_ERRINFO(returnCode, "Failed to set SIU loopback"));
}

void CrorcChannelMaster::crorcStartTrigger()
{
  stword_t stw;
  int returnCode = rorcStartTrigger(getBarUserspace(), DDL_RESPONSE_TIME * mPciLoopPerUsec, &stw);
  THROW_IF_BAD_STATUS(returnCode, CrorcStartTriggerException()
      ADD_ERRINFO(returnCode, "Failed to start trigger"));
}

void CrorcChannelMaster::crorcStopTrigger()
{
  stword_t stw;
  uint64_t timeout = mPciLoopPerUsec * DDL_RESPONSE_TIME;
  int returnCode = rorcStopTrigger(getBarUserspace(), timeout, &stw);
  THROW_IF_BAD_STATUS(returnCode, CrorcStopTriggerException()
      ADD_ERRINFO(returnCode, "Failed to stop trigger"));
}

std::vector<uint32_t> CrorcChannelMaster::utilityCopyFifo()
{
  std::vector<uint32_t> copy;
  auto& fifo = getFifoUser()->dataInt32;
  copy.insert(copy.begin(), fifo.begin(), fifo.end());
  return copy;
}

void CrorcChannelMaster::utilityPrintFifo(std::ostream& os)
{
  ChannelUtility::printCrorcFifo(getFifoUser(), os);
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

