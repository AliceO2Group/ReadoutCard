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
    : ChannelMasterPdaBase(parameters, allowedChannels(), sizeof(ReadyFifo)), //
    mPageSize(parameters.getDmaPageSize().get_value_or(8*1024)), // 8 kB default for uniformity with CRU
    mInitialResetLevel(ResetLevel::Rorc), // It's good to reset at least the card channel in general
    mNoRDYRX(true), // Not sure
    mUseFeeAddress(false), // Not sure
    mLoopbackMode(parameters.getGeneratorLoopback().get_value_or(LoopbackMode::Rorc)), // Internal loopback by default
    mGeneratorEnabled(parameters.getGeneratorEnabled().get_value_or(true)), // Use data generator by default
    mGeneratorPattern(parameters.getGeneratorPattern().get_value_or(GeneratorPattern::Incremental)), //
    mGeneratorMaximumEvents(0), // Infinite events
    mGeneratorInitialValue(0), // Start from 0
    mGeneratorInitialWord(0), // First word
    mGeneratorSeed(mGeneratorPattern == GeneratorPattern::Random ? 1 : 0), // We use a seed for random only
    mGeneratorDataSize(parameters.getGeneratorDataSize().get_value_or(mPageSize)), // Can use page size
    mUseContinuousReadout(parameters.getReadoutMode().is_initialized() ?
            parameters.getReadoutModeRequired() == ReadoutMode::Continuous : false)
{
  // Create and register our ReadyFIFO buffer
  log("Initializing ReadyFIFO DMA buffer", InfoLogger::InfoLogger::Debug);
  {
    // Create and register the buffer
    // Note: if resizing the file fails, we might've accidentally put the file in a hugetlbfs mount with 1 GB page size
    constexpr auto FIFO_SIZE = sizeof(ReadyFifo);
    Utilities::resetSmartPtr(mBufferFifoFile, getPaths().fifo(), FIFO_SIZE);
    Utilities::resetSmartPtr(mPdaDmaBufferFifo, getRorcDevice().getPciDevice(), mBufferFifoFile->getAddress(),
        FIFO_SIZE, getPdaDmaBufferIndexFifo(getChannelNumber()));

    const auto& entry = mPdaDmaBufferFifo->getScatterGatherList().at(0);
    if (entry.size < FIFO_SIZE) {
      // Something must've failed at some point
      BOOST_THROW_EXCEPTION(Exception()
          << ErrorInfo::Message("Scatter gather list entry for internal FIFO was too small")
          << ErrorInfo::ScatterGatherEntrySize(entry.size)
          << ErrorInfo::FifoSize(FIFO_SIZE));
    }
    mReadyFifoAddressUser = entry.addressUser;
    mReadyFifoAddressBus = entry.addressBus;
  }

  getReadyFifoUser()->reset();
  mDmaBufferUserspace = getPdaDmaBuffer().getScatterGatherList().at(0).addressUser;
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

  if (mUseContinuousReadout) {
    log("Initializing continuous readout");
    crorcInitReadoutContinuous();
  }

  // Find DIU version, required for armDdl()
  crorcInitDiuVersion();

  // Resetting the card,according to the RESET LEVEL parameter
  deviceResetChannel(mInitialResetLevel);

  // Setting the card to be able to receive data
  startDataReceiving();

  // Initializing the firmware FIFO, pushing (entries) pages
  for(int i = 0; i < READYFIFO_ENTRIES; ++i){
    getReadyFifoUser()->entries[i].reset();
    pushIntoSuperpage(entry);
  }

  assert(entry.pushedPages <= entry.maxPages);
  if (entry.pushedPages == entry.maxPages) {
    // Remove superpage from pushing queue
    mSuperpageQueue.removeFromPushingQueue();
  }

  if (mGeneratorEnabled) {
    log("Starting data generator");
    // Starting the data generator
    startDataGenerator();
  } else {
    if (!mNoRDYRX) {
      log("Starting trigger");

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

  entry.superpage.received += READYFIFO_ENTRIES * mPageSize;

  if (entry.superpage.getReceived() == entry.superpage.getSize()) {
    mSuperpageQueue.moveFromArrivalsToFilledQueue();
  }

  getReadyFifoUser()->reset();
  mFifoBack = 0;
  mFifoSize = 0;

  mPendingDmaStart = false;
  log("DMA started");

  if (mUseContinuousReadout) {
    log("Starting continuous readout");
    crorcStartReadoutContinuous();
  }
}

void CrorcChannelMaster::deviceStopDma()
{
  if (mGeneratorEnabled) {
    // Starting the data generator
    startDataGenerator();
    crorcStopDataGenerator();
    rorcStopDataReceiver(getBarUserspace());
  } else {
    if (!mNoRDYRX) {
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

  try {
    if (resetLevel == ResetLevel::Rorc) {
      crorcReset(RORC_RESET_FF);
      crorcReset(RORC_RESET_RORC);
    }

    if (LoopbackMode::isExternal(mLoopbackMode)) {
      crorcArmDdl(RORC_RESET_DIU);

      if ((resetLevel == ResetLevel::RorcDiuSiu) && (mLoopbackMode != LoopbackMode::Diu))
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
    e << ErrorInfo::LoopbackMode(mLoopbackMode);
    throw;
  }

  // Wait a little after reset.
  std::this_thread::sleep_for(100ms); /// XXX Why???
}

void CrorcChannelMaster::startDataGenerator()
{
  if (LoopbackMode::None == mLoopbackMode) {
    crorcStartTrigger();
  }

  crorcArmDataGenerator();

  if (LoopbackMode::Rorc == mLoopbackMode) {
    rorcParamOn(getBarUserspace(), PRORC_PARAM_LOOPB);
    std::this_thread::sleep_for(100ms); // XXX Why???
  }

  if (LoopbackMode::Siu == mLoopbackMode) {
    crorcSetSiuLoopback();
    std::this_thread::sleep_for(100ms); // XXX Why???
    crorcCheckLink();
    crorcSiuCommand(RandCIFST);
    crorcDiuCommand(RandCIFST);
  }

  rorcStartDataGenerator(getBarUserspace(), mGeneratorMaximumEvents);
}

void CrorcChannelMaster::startDataReceiving()
{
  crorcInitDiuVersion();

  // Preparing the card.
  if (LoopbackMode::Siu == mLoopbackMode) {
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

auto CrorcChannelMaster::getSuperpage() -> Superpage
{
  return mSuperpageQueue.getFrontSuperpage();
}

void CrorcChannelMaster::pushSuperpage(Superpage superpage)
{
  checkSuperpage(superpage);

  SuperpageQueueEntry entry;
  entry.busAddress = getBusOffsetAddress(superpage.getOffset());
  entry.maxPages = superpage.getSize() / mPageSize;
  entry.pushedPages = 0;
  entry.superpage = superpage;
  entry.superpage.received = 0;

  mSuperpageQueue.addToQueue(entry);
}

auto CrorcChannelMaster::popSuperpage() -> Superpage
{
  return mSuperpageQueue.removeFromFilledQueue().superpage;
}

void CrorcChannelMaster::fillSuperpages()
{
  // Push new pages into superpage
  if (!mSuperpageQueue.getPushing().empty()) {
    SuperpageQueueEntry& entry = mSuperpageQueue.getPushingFrontEntry();

    if (mPendingDmaStart) {
      // Do some special handling of first transfers......
      startPendingDma(entry);
    } else {
      int freeDescriptors = FIFO_QUEUE_MAX - mFifoSize;
      int freePages = entry.getUnpushedPages();
      int possibleToPush = std::min(freeDescriptors, freePages);

      for (int i = 0; i < possibleToPush; ++i) {
        pushIntoSuperpage(entry);
      }

      if (entry.isPushed()) {
        // Remove superpage from pushing queue
        mSuperpageQueue.removeFromPushingQueue();
      }
    }
  }

  // Check for arrivals & handle them
  if (!mSuperpageQueue.getArrivals().empty()) {
    auto isArrived = [&](int descriptorIndex) {return dataArrived(descriptorIndex) == DataArrivalStatus::WholeArrived;};
    auto resetDescriptor = [&](int descriptorIndex) {getReadyFifoUser()->entries[descriptorIndex].reset();};

    while (mFifoSize > 0) {
      SuperpageQueueEntry& entry = mSuperpageQueue.getArrivalsFrontEntry();

      if (isArrived(mFifoBack)) {
        if (false) {
          // XXX Dirty hack for now: write length field into page SDH. In upcoming firmwares, the card will do this
          // itself
          auto writeSdhEventSize = [](uintptr_t pageAddress, uint32_t eventSize){
            constexpr size_t OFFSET_SDH_EVENT_SIZE = 16; // 1 * 128b word
            auto address = reinterpret_cast<char*>(pageAddress + OFFSET_SDH_EVENT_SIZE);
            // Clear first 3 32b values of event size word
            memset(address, 0, sizeof(uint32_t) * 3);
            // Write to 4th 32b value of event size word
            memcpy(address + (sizeof(uint32_t) * 3), &eventSize, sizeof(uint32_t));
          };

          uint32_t length = getReadyFifoUser()->entries[mFifoBack].length;
          auto pageAddress = mDmaBufferUserspace + entry.superpage.getOffset() + entry.superpage.received;
          writeSdhEventSize(pageAddress, length);
        }

        resetDescriptor(mFifoBack);
        mFifoSize--;
        mFifoBack = (mFifoBack + 1) % READYFIFO_ENTRIES;
        entry.superpage.received += mPageSize;

        if (entry.superpage.isFilled()) {
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

uintptr_t CrorcChannelMaster::getNextSuperpageBusAddress(const SuperpageQueueEntry& superpage)
{
  auto pageSize = mPageSize;
  auto offset = pageSize * superpage.pushedPages;
  uintptr_t pageBusAddress = superpage.busAddress + offset;
  return pageBusAddress;
}

void CrorcChannelMaster::pushFreeFifoPage(int readyFifoIndex, uintptr_t pageBusAddress)
{
  size_t pageWords = mPageSize / 4; // Size in 32-bit words
  rorcPushRxFreeFifo(getBarUserspace(), pageBusAddress, pageWords, readyFifoIndex);
}

CrorcChannelMaster::DataArrivalStatus::type CrorcChannelMaster::dataArrived(int index)
{
  auto length = getReadyFifoUser()->entries[index].length;
  auto status = getReadyFifoUser()->entries[index].status;

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
  int roundedLen = -1;
  int returnCode = rorcArmDataGenerator(getBarUserspace(), mGeneratorInitialValue, mGeneratorInitialWord,
      mGeneratorPattern, mGeneratorDataSize / 4, mGeneratorSeed, &roundedLen);

  THROW_IF_BAD_STATUS(returnCode, CrorcArmDataGeneratorException()
      ADD_ERRINFO(returnCode, "Failed to arm data generator")
      << ErrorInfo::GeneratorPattern(mGeneratorPattern)
      << ErrorInfo::GeneratorEventLength(mGeneratorDataSize / 4));
}

void CrorcChannelMaster::crorcStopDataGenerator()
{
  rorcWriteReg(getBarUserspace(), C_CSR, DRORC_CMD_STOP_DG);
}

void CrorcChannelMaster::crorcStopDataReceiver()
{
  rorcStopDataReceiver(getBarUserspace());
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
  if (returnCode != RORC_FF_EMPTY) {
    BOOST_THROW_EXCEPTION(
        CrorcFreeFifoException() ADD_ERRINFO(returnCode, "Free FIFO not empty")
        << ErrorInfo::PossibleCauses({"Previous DMA did not get/free all received pages"}));
  }
}

void CrorcChannelMaster::crorcStartDataReceiver()
{
  rorcStartDataReceiver(getBarUserspace(), getReadyFifoAddressBus(), mRorcRevision);
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

void CrorcChannelMaster::crorcInitReadoutContinuous()
{
  // Based on:
  // https://gitlab.cern.ch/costaf/grorc_sw/blob/master/script/grorc_cont_readout.sh

  // this script works with the G-RORC firmware 3.16
  // SET the card in CONT MODE
  getBar2().setRegister(0x190, 0x02000000);
  // s_send_wide            <= s_reg(0);
  // s_gbt_tx_pol           <= s_reg(1);
  // s_gbt_tx_sel           <= s_reg(3 downto 2);
  // s_gbt_reset            <= s_reg(4);
  // s_rx_encoding          <= s_reg(8);
  // s_tx_encoding          <= s_reg(9);
  // s_read_gate            <= s_reg(12);
  // s_disable_bank         <= s_reg(17 downto 16);
  // s_bar_rd_ch(2).reg_194 <= s_reg;
  // Choose only BANK0
  getBar2().setRegister(0x194, 0x21005);
  // number of GBT words per event
  getBar2().setRegister(0x18c, 0x1f);
}

void CrorcChannelMaster::crorcStartReadoutContinuous()
{
  // Based on:
  // https://gitlab.cern.ch/costaf/grorc_sw/blob/master/script/grorc_send_IDLE_SYNC.sh

  // SET IT AS CUSTOM PATTERN FOR 1 CC and open the GATE READOUT
  getBar2().setRegister(0x198, 0x60000001);
  getBar2().setRegister(0x198, 0x0);
}

void CrorcChannelMaster::crorcInitReadoutTriggered()
{
  throw std::runtime_error("not implemented");
}

Pda::PdaBar& CrorcChannelMaster::getBar2()
{
  if (!mPdaBar2) {
    Utilities::resetSmartPtr(mPdaBar2, getRorcDevice().getPciDevice(), 2);
  }
  return *(mPdaBar2.get());
}

std::vector<uint32_t> CrorcChannelMaster::utilityCopyFifo()
{
  std::vector<uint32_t> copy;
  auto& fifo = getReadyFifoUser()->dataInt32;
  copy.insert(copy.begin(), fifo.begin(), fifo.end());
  return copy;
}

void CrorcChannelMaster::utilityPrintFifo(std::ostream& os)
{
  ChannelUtility::printCrorcFifo(getReadyFifoUser(), os);
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
  ChannelUtility::crorcCleanupState(ChannelPaths(getCardDescriptor().pciAddress, getChannelNumber()));
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

std::string CrorcChannelMaster::utilityGetFirmwareVersionString()
{
  uint32_t version = readRegister(2);
  auto bits = [&](int lsb, int msb) { return Utilities::getBits(version, lsb, msb); };

  uint32_t reserved = bits(24, 31);
  uint32_t major = bits(20, 23);
  uint32_t minor = bits(13, 19);
  uint32_t year = bits(9, 12) + 2000;
  uint32_t month = bits(5, 8);
  uint32_t day = bits(0, 4);

  if (reserved != 0x2) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << ErrorInfo::Message("Static field of version register did not equal 0x2"));
  }

  std::ostringstream stream;
  stream << major << '.' << minor << ':' << year << '-' << month << '-' << day;
  return stream.str();
}

} // namespace Rorc
} // namespace AliceO2

