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
#include <boost/circular_buffer.hpp>
#include <boost/format.hpp>
#include "c/rorc/rorc.h"
#include "c/rorc/ddl_def.h"
#include "ChannelPaths.h"
#include "ChannelUtilityImpl.h"
#include "RorcStatusCode.h"
#include "Utilities/SmartPointer.h"

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
    << ErrorInfo::Message(_err_message) \
    << ErrorInfo::StatusCode(_status_code)

CrorcChannelMaster::CrorcChannelMaster(const Parameters& parameters)
    : ChannelMasterPdaBase(CARD_TYPE, parameters, allowedChannels(), sizeof(ReadyFifo))
{
//  auto& params = getChannelParameters();

  getFifoUser()->reset();

  // TODO reimplement check
//  if (getPageAddresses().size() <= READYFIFO_ENTRIES) {
//    BOOST_THROW_EXCEPTION(CrorcException()
//        << ErrorInfo::Message("Insufficient amount of pages fit in DMA buffer")
//        << ErrorInfo::Pages(getPageAddresses().size())
//        << ErrorInfo::DmaBufferSize(params.dma.bufferSize)
//        << ErrorInfo::DmaPageSize(params.dma.pageSize));
//  }
}

auto CrorcChannelMaster::allowedChannels() -> AllowedChannels {
  return {0, 1, 2, 3, 4, 5};
}

CrorcChannelMaster::~CrorcChannelMaster()
{
  deviceStopDma();
}

void CrorcChannelMaster::deviceStartDma()
{
  log("DMA start deferred until superpage available");

  mFifoBack = 0;
  mFifoSize = 0;
  mSuperpageQueue.clear();
  mPendingDmaStart = true;
}

void CrorcChannelMaster::startPendingDma(SuperpageQueueEntry& entry)
{
  if (!mPendingDmaStart) {
    return;
  }

  log("Starting pending DMA");

  auto& params = getChannelParameters();

  // Find DIU version, required for armDdl()
  crorcInitDiuVersion();

  // Resetting the card,according to the RESET LEVEL parameter
  deviceResetChannel(params.initialResetLevel);

  // Setting the card to be able to receive data
  startDataReceiving();

  // Initializing the firmware FIFO, pushing (entries) pages
  getFifoUser()->reset();

  for(int i = 0; i < READYFIFO_ENTRIES; ++i){
//    getFifoUser()->entries[i].reset();
    pushIntoSuperpage(entry);
  }

  assert(entry.pushedPages <= entry.status.maxPages);
  if (entry.pushedPages == entry.status.maxPages) {
    // Remove superpage from pushing queue
    mSuperpageQueue.removeFromPushingQueue();
  }

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

  /// Fixed wait for initial pages TODO polling wait with timeout
  std::this_thread::sleep_for(10ms);
  if (dataArrived(READYFIFO_ENTRIES - 1) != DataArrivalStatus::WholeArrived) {
    log("Initial pages not arrived", InfoLogger::InfoLogger::Warning);
  }

  entry.status.confirmedPages += READYFIFO_ENTRIES;

  if (entry.status.confirmedPages == entry.status.maxPages) {
    mSuperpageQueue.moveFromArrivalsToFilledQueue();
  }

  getFifoUser()->reset();
  mFifoBack = 0;
  mFifoSize = 0;

  mPendingDmaStart = false;
  log("DMA started");
}

int rorcStopDataGenerator(volatile uint32_t* buff)
{
  rorcWriteReg(buff, C_CSR, DRORC_CMD_STOP_DG);
  return RORC_STATUS_OK;
}

void CrorcChannelMaster::deviceStopDma()
{
  auto& params = getChannelParameters();
  if (params.generator.useDataGenerator) {
    // Starting the data generator
    startDataGenerator(params.generator);
    rorcStopDataGenerator(getBarUserspace());
    rorcStopDataReceiver(getBarUserspace());
  } else {
    if (!params.noRDYRX) {
      // Sending EOBTR to FEE.
      crorcStopTrigger();
    }
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
    e << ErrorInfo::ResetLevel(resetLevel);
    e << ErrorInfo::LoopbackMode(loopbackMode);
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
  std::this_thread::sleep_for(10ms); /// XXX Give card some time to reset the FreeFIFO
  crorcCheckFreeFifoEmpty();
  crorcStartDataReceiver();
}

void CrorcChannelMaster::initializeFreeFifo()
{
  // Pushing a given number of pages to the firmware FIFO.

  // TODO Now we switched to user-provided buffer, we must somehow find a place to store the initial pages
  // Probably should delay this until we get superpages?
  // For now, we use start of DMA buffer...
  volatile void* junkAddress = getBusOffsetAddress(getBufferProvider().getDmaOffset());

  for(int i = 0; i < READYFIFO_ENTRIES; ++i){
    getFifoUser()->entries[i].reset();
    pushFreeFifoPage(i, junkAddress);
  }
}

int CrorcChannelMaster::getSuperpageQueueCount()
{
  return mSuperpageQueue.getQueueCount();
}

int CrorcChannelMaster::getSuperpageQueueAvailable()
{
  return mSuperpageQueue.getQueueAvailable();
}

int CrorcChannelMaster::getSuperpageQueueCapacity()
{
  return mSuperpageQueue.getQueueCapacity();
}

auto CrorcChannelMaster::getSuperpageStatus() -> SuperpageStatus
{
  return mSuperpageQueue.getFrontSuperpageStatus();
}

void CrorcChannelMaster::pushSuperpage(size_t offset, size_t size)
{
  if (mSuperpageQueue.isFull()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, queue at capacity"));
  }

  if (size == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, size == 0"));
  }

  if ((size % 1024*1024) != 0) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Could not enqueue superpage, size not a multiple of 1 MiB"));
  }

  // TODO check if offset is properly aligned

  SuperpageQueueEntry entry;
  entry.busAddress = getBusOffsetAddress(offset + getBufferProvider().getDmaOffset());
  entry.pushedPages = 0;
  entry.status.confirmedPages = 0;
  entry.status.maxPages = size / getChannelParameters().dma.pageSize;
  entry.status.offset = offset;

  mSuperpageQueue.addToQueue(entry);
}

auto CrorcChannelMaster::popSuperpage() -> SuperpageStatus
{
  return mSuperpageQueue.removeFromFilledQueue().status;
}

void CrorcChannelMaster::fillSuperpages()
{
  // Push new pages into superpage
  if (!mSuperpageQueue.getPushing().empty()) {
    auto offset = mSuperpageQueue.getPushing().front();
    SuperpageQueueEntry& entry = mSuperpageQueue.getEntry(offset);

    if (mPendingDmaStart) {
      // Do some special handling of first transfers......
      startPendingDma(entry);
    } else {
      int freeDescriptors = FIFO_QUEUE_MAX - mFifoSize;
      int freePages = entry.status.maxPages - entry.pushedPages;
      int possibleToPush = std::min(freeDescriptors, freePages);

      for (int i = 0; i < possibleToPush; ++i) {
        pushIntoSuperpage(entry);
      }

      if (entry.pushedPages == entry.status.maxPages) {
        // Remove superpage from pushing queue
        mSuperpageQueue.removeFromPushingQueue();
      }
    }
  }

  // Check for arrivals & handle them
  if (!mSuperpageQueue.getArrivals().empty()) {
    auto isArrived = [&](int descriptorIndex) {return dataArrived(descriptorIndex) == DataArrivalStatus::WholeArrived;};
    auto resetDescriptor = [&](int descriptorIndex) {getFifoUser()->entries[descriptorIndex].reset();};

    while (mFifoSize > 0) {
      auto offset = mSuperpageQueue.getArrivals().front();
      SuperpageQueueEntry& entry = mSuperpageQueue.getEntry(offset);

      if (isArrived(mFifoBack)) {
//        printf("        is arrived\n");

        resetDescriptor(mFifoBack);
        mFifoSize--;
        mFifoBack = (mFifoBack + 1) % READYFIFO_ENTRIES;
        entry.status.confirmedPages++;

        if (entry.status.confirmedPages == entry.status.maxPages) {
          // Move superpage to filled queue
          mSuperpageQueue.moveFromArrivalsToFilledQueue();
        }
      } else {
        // If the back one hasn't arrived yet, the next ones will certainly not have arrived either...
        break;
      }
    }
  }
}

void CrorcChannelMaster::pushIntoSuperpage(SuperpageQueueEntry& superpage)
{
  assert(mFifoSize < FIFO_QUEUE_MAX);
  assert(superpage.pushedPages < superpage.status.maxPages);

  pushFreeFifoPage(getFifoFront(), getNextSuperpageBusAddress(superpage));
  mFifoSize++;
  superpage.pushedPages++;
}

volatile void* CrorcChannelMaster::getNextSuperpageBusAddress(const SuperpageQueueEntry& superpage)
{
  auto pageSize = getChannelParameters().dma.pageSize;
  auto offset = pageSize * superpage.pushedPages;
  volatile void* pageBusAddress = reinterpret_cast<volatile void*>(
      reinterpret_cast<volatile char*>(superpage.busAddress) + offset);
  return pageBusAddress;
}

void CrorcChannelMaster::pushFreeFifoPage(int readyFifoIndex, volatile void* pageBusAddress)
{
  size_t pageWords = getChannelParameters().dma.pageSize / 4; // Size in 32-bit words
  rorcPushRxFreeFifo(getBarUserspace(), reinterpret_cast<uint64_t>(pageBusAddress), pageWords, readyFifoIndex);
}

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
          << ErrorInfo::Message("Data arrival status word contains error bits")
          << ErrorInfo::ReadyFifoStatus(status)
          << ErrorInfo::ReadyFifoLength(length)
          << ErrorInfo::FifoIndex(index));
    }
    return DataArrivalStatus::WholeArrived;
  }

  BOOST_THROW_EXCEPTION(CrorcDataArrivalException()
      << ErrorInfo::Message("Unrecognized data arrival status word")
      << ErrorInfo::ReadyFifoStatus(status)
      << ErrorInfo::ReadyFifoLength(length)
      << ErrorInfo::FifoIndex(index));
}

CardType::type CrorcChannelMaster::getCardType()
{
  return CardType::Crorc;
}

void CrorcChannelMaster::crorcArmDataGenerator()
{
  auto& gen = getChannelParameters().generator;
  int roundedLen;
  int returnCode = rorcArmDataGenerator(getBarUserspace(), gen.initialValue, gen.initialWord, gen.pattern,
      gen.dataSize / 4, gen.seed, &roundedLen);
  THROW_IF_BAD_STATUS(returnCode, CrorcArmDataGeneratorException()
      ADD_ERRINFO(returnCode, "Failed to arm data generator")
      << ErrorInfo::GeneratorPattern(gen.pattern)
      << ErrorInfo::GeneratorEventLength(gen.dataSize / 4));
}

void CrorcChannelMaster::crorcArmDdl(int resetMask)
{
  int returnCode = rorcArmDDL(getBarUserspace(), resetMask, mDiuVersion, mPciLoopPerUsec);
  THROW_IF_BAD_STATUS(returnCode, CrorcArmDdlException()
      ADD_ERRINFO(returnCode, "Failed to arm DDL")
      << ErrorInfo::DdlResetMask(b::str(b::format("0x%x") % resetMask)));
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
      << ErrorInfo::SiuCommand(command));
}

void CrorcChannelMaster::crorcDiuCommand(int command)
{
  int returnCode = ddlReadDiu(getBarUserspace(), command, DDL_RESPONSE_TIME, mPciLoopPerUsec);
  THROW_IF_BAD_STATUS(returnCode, CrorcSiuCommandException()
      ADD_ERRINFO(returnCode, "Failed to send DIU command")
      << ErrorInfo::DiuCommand(command));
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
        << ErrorInfo::PossibleCauses({"Previous DMA did not get/free all received pages"}));
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
  BOOST_THROW_EXCEPTION(CrorcException() << ErrorInfo::Message("C-RORC does not support setting LED state"));
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

