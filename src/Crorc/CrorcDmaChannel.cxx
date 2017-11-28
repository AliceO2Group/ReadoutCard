/// \file CrorcDmaChannel.cxx
/// \brief Implementation of the CrorcDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CrorcDmaChannel.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <boost/circular_buffer.hpp>
#include <boost/format.hpp>
#include "ChannelPaths.h"
#include "Crorc/Constants.h"
#include "Utilities/SmartPointer.h"

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
using std::cout;
using std::endl;
using namespace std::literals;

namespace AliceO2 {
namespace roc {

CrorcDmaChannel::CrorcDmaChannel(const Parameters& parameters)
    : DmaChannelPdaBase(parameters, allowedChannels()), //
    mPdaBar(getRocPciDevice().getPciDevice(), getChannelNumber()), // Initialize main DMA channel BAR
    mPdaBar2(getRocPciDevice().getPciDevice(), 2), // Initialize BAR 2
    mPageSize(parameters.getDmaPageSize().get_value_or(8*1024)), // 8 kB default for uniformity with CRU
    mInitialResetLevel(ResetLevel::Internal), // It's good to reset at least the card channel in general
    mNoRDYRX(true), // Not sure
    mUseFeeAddress(false), // Not sure
    mLoopbackMode(parameters.getGeneratorLoopback().get_value_or(LoopbackMode::Internal)), // Internal loopback by default
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
    Utilities::resetSmartPtr(mBufferFifoFile, getPaths().fifo(), FIFO_SIZE, true);
    Utilities::resetSmartPtr(mPdaDmaBufferFifo, getRocPciDevice().getPciDevice(), mBufferFifoFile->getAddress(),
        FIFO_SIZE, getPdaDmaBufferIndexFifo(getChannelNumber()), false);// note the 'false' at the end specifies non-hugepage memory

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
  mDmaBufferUserspace = getBufferProvider().getAddress();
}

auto CrorcDmaChannel::allowedChannels() -> AllowedChannels {
  return {0, 1, 2, 3, 4, 5};
}

CrorcDmaChannel::~CrorcDmaChannel()
{
  deviceStopDma();
}

void CrorcDmaChannel::deviceStartDma()
{
  log("DMA start deferred until enough superpages available");

  mFifoBack = 0;
  mFifoSize = 0;
  mReadyQueue.clear();
  mTransferQueue.clear();
  mPendingDmaStart = true;
}

void CrorcDmaChannel::startPendingDma()
{
  if (!mPendingDmaStart) {
    return;
  }

  if (mTransferQueue.size() < DMA_START_REQUIRED_SUPERPAGES) {
    log("Insufficient superpages to start pending DMA");
    return;
  }

  log("Starting pending DMA");

  if (mUseContinuousReadout) {
    log("Initializing continuous readout");
    Crorc::Crorc::initReadoutContinuous(mPdaBar2);
  }

  // Find DIU version, required for armDdl()
  mDiuConfig = getCrorc().initDiuVersion();

  // Resetting the card,according to the RESET LEVEL parameter
  deviceResetChannel(mInitialResetLevel);

  // Setting the card to be able to receive data
  startDataReceiving();

  // Initializing the firmware FIFO, pushing (entries) pages
  for(int i = 0; i < DMA_START_REQUIRED_SUPERPAGES; ++i){
    getReadyFifoUser()->entries[i].reset();
    auto superpage = mTransferQueue[i];
    mReadyQueue.push_back();
    pushFreeFifoPage(i, getBusOffsetAddress(superpage.offset));
  }

  if (mGeneratorEnabled) {
    log("Starting data generator");
    // Starting the data generator
    startDataGenerator();
  } else {
    if (!mNoRDYRX) {
      log("Starting trigger");

      // Clearing SIU/DIU status.
      getCrorc().assertLinkUp();
      getCrorc().siuCommand(Ddl::RandCIFST);
      getCrorc().diuCommand(Ddl::RandCIFST);

      // RDYRX command to FEE
      getCrorc().startTrigger(mDiuConfig);
    }
  }

  /// Fixed wait for initial pages TODO polling wait with timeout
  std::this_thread::sleep_for(10ms);
  if (dataArrived(READYFIFO_ENTRIES - 1) != DataArrivalStatus::WholeArrived) {
    log("Initial pages not arrived", InfoLogger::InfoLogger::Warning);
  }

  for(int i = 0; i < DMA_START_REQUIRED_SUPERPAGES; ++i){
    auto superpage = mTransferQueue.front();
    mReadyQueue.push_back(superpage);
    mTransferQueue.pop_front();
  }

  getReadyFifoUser()->reset();
  mFifoBack = 0;
  mFifoSize = 0;

  mPendingDmaStart = false;
  log("DMA started");

  if (mUseContinuousReadout) {
    log("Starting continuous readout");
    Crorc::Crorc::startReadoutContinuous(mPdaBar2);
  }
}

void CrorcDmaChannel::deviceStopDma()
{
  if (mGeneratorEnabled) {
    // Starting the data generator
    startDataGenerator();
    getCrorc().stopDataGenerator();
    getCrorc().stopDataReceiver();
  } else {
    if (!mNoRDYRX) {
      // Sending EOBTR to FEE.
      getCrorc().stopTrigger(mDiuConfig);
    }
  }
}

void CrorcDmaChannel::deviceResetChannel(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::Nothing) {
    return;
  }

  try {
    if (resetLevel == ResetLevel::Internal) {
      getCrorc().resetCommand(Rorc::Reset::FF, mDiuConfig);
      getCrorc().resetCommand(Rorc::Reset::RORC, mDiuConfig);
    }

    if (LoopbackMode::isExternal(mLoopbackMode)) {
      getCrorc().armDdl(Rorc::Reset::DIU, mDiuConfig);

      if ((resetLevel == ResetLevel::InternalDiuSiu) && (mLoopbackMode != LoopbackMode::Diu))
      {
        // Wait a little before SIU reset.
        std::this_thread::sleep_for(100ms); /// XXX Why???
        // Reset SIU.
        getCrorc().armDdl(Rorc::Reset::SIU, mDiuConfig);
        getCrorc().armDdl(Rorc::Reset::DIU, mDiuConfig);
      }

      getCrorc().armDdl(Rorc::Reset::RORC, mDiuConfig);
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

void CrorcDmaChannel::startDataGenerator()
{
  if (LoopbackMode::None == mLoopbackMode) {
    getCrorc().startTrigger(mDiuConfig);
  }

  getCrorc().armDataGenerator(mGeneratorInitialValue, mGeneratorInitialWord, mGeneratorPattern, mGeneratorDataSize,
      mGeneratorSeed);

  if (LoopbackMode::Internal == mLoopbackMode) {
    getCrorc().setLoopbackOn();
    std::this_thread::sleep_for(100ms); // XXX Why???
  }

  if (LoopbackMode::Siu == mLoopbackMode) {
    getCrorc().setSiuLoopback(mDiuConfig);
    std::this_thread::sleep_for(100ms); // XXX Why???
    getCrorc().assertLinkUp();
    getCrorc().siuCommand(Ddl::RandCIFST);
    getCrorc().diuCommand(Ddl::RandCIFST);
  }

  getCrorc().startDataGenerator(mGeneratorMaximumEvents);
}

void CrorcDmaChannel::startDataReceiving()
{
  getCrorc().initDiuVersion();

  // Preparing the card.
  if (LoopbackMode::Siu == mLoopbackMode) {
    deviceResetChannel(ResetLevel::InternalDiuSiu);
    getCrorc().assertLinkUp();
    getCrorc().siuCommand(Ddl::RandCIFST);
    getCrorc().diuCommand(Ddl::RandCIFST);
  }

  getCrorc().resetCommand(Rorc::Reset::FF, mDiuConfig);
  std::this_thread::sleep_for(10ms); /// XXX Give card some time to reset the FreeFIFO
  getCrorc().assertFreeFifoEmpty();
  getCrorc().startDataReceiver(mReadyFifoAddressBus);
}

int CrorcDmaChannel::getTransferQueueAvailable()
{
  return TRANSFER_QUEUE_CAPACITY - mTransferQueue.size();
}

int CrorcDmaChannel::getReadyQueueSize()
{
  return mReadyQueue.size();
}

auto CrorcDmaChannel::getSuperpage() -> Superpage
{
  if (mReadyQueue.empty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get superpage, ready queue was empty"));
  }
  return mReadyQueue.front();
}

void CrorcDmaChannel::pushSuperpage(Superpage superpage)
{
  checkSuperpage(superpage);
  constexpr size_t MIN_SIZE = 1*1024*1024;
  constexpr size_t MAX_SIZE = 2*1024*1024;

  if (superpage.getSize() > MAX_SIZE) {
    BOOST_THROW_EXCEPTION(CrorcException()
      << ErrorInfo::Message("Could not enqueue superpage, C-RORC backend does not support superpage sizes above 2 MiB"));
  }

  if (!Utilities::isMultiple(superpage.getSize(), MIN_SIZE)) {
    BOOST_THROW_EXCEPTION(CrorcException()
      << ErrorInfo::Message("Could not enqueue superpage, C-RORC backend requires superpage size multiple of 1 MiB"));
  }

  if (mTransferQueue.size() >= TRANSFER_QUEUE_CAPACITY) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
  }

  if (mFifoSize >= READYFIFO_ENTRIES) {
    BOOST_THROW_EXCEPTION(Exception()
      << ErrorInfo::Message("Could not push superpage, firmware queue was full (this should never happen)"));
  }

  mTransferQueue.push_back(superpage);
  auto busAddress = getBusOffsetAddress(superpage.getOffset());
  pushFreeFifoPage(getFifoFront(), busAddress);
  mFifoSize++;
}

auto CrorcDmaChannel::popSuperpage() -> Superpage
{
  if (mReadyQueue.empty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, ready queue was empty"));
  }
  auto superpage = mReadyQueue.front();
  mReadyQueue.pop_front();
  return superpage;
}

void CrorcDmaChannel::fillSuperpages()
{
  if (mPendingDmaStart) {
    if (mTransferQueue.size() >= DMA_START_REQUIRED_SUPERPAGES) {
      startPendingDma();
    } else {
      // Waiting on enough superpages to start DMA...
      return;
    }
  }

  // Check for arrivals & handle them
  if (!mTransferQueue.empty()) {
    auto isArrived = [&](int descriptorIndex) {return dataArrived(descriptorIndex) == DataArrivalStatus::WholeArrived;};
    auto resetDescriptor = [&](int descriptorIndex) {getReadyFifoUser()->entries[descriptorIndex].reset();};

    while (mFifoSize > 0) {
      if (isArrived(mFifoBack)) {
        uint32_t length = getReadyFifoUser()->entries[mFifoBack].getSize();
        resetDescriptor(mFifoBack);

        mFifoSize--;
        mFifoBack = (mFifoBack + 1) % READYFIFO_ENTRIES;

        auto superpage = mTransferQueue.front();
        superpage.received = length;
        superpage.ready = true;
        mReadyQueue.push_back(superpage);
        mTransferQueue.pop_front();
      } else {
        // If the back one hasn't arrived yet, the next ones will certainly not have arrived either...
        break;
      }
    }
  }
}

void CrorcDmaChannel::pushFreeFifoPage(int readyFifoIndex, uintptr_t pageBusAddress)
{
  size_t pageWords = mPageSize / 4; // Size in 32-bit words
  getCrorc().pushRxFreeFifo(pageBusAddress, pageWords, readyFifoIndex);
}

CrorcDmaChannel::DataArrivalStatus::type CrorcDmaChannel::dataArrived(int index)
{
  auto length = getReadyFifoUser()->entries[index].length;
  auto status = getReadyFifoUser()->entries[index].status;

  if (status == -1) {
    return DataArrivalStatus::NoneArrived;
  } else if (status == 0) {
    return DataArrivalStatus::PartArrived;
  } else if ((status & 0xff) == Ddl::DTSW) {
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

CardType::type CrorcDmaChannel::getCardType()
{
  return CardType::Crorc;
}

boost::optional<int32_t> CrorcDmaChannel::getSerial()
{
  return Crorc::getSerial(mPdaBar);
}

boost::optional<std::string> CrorcDmaChannel::getFirmwareInfo()
{
  uint32_t version = mPdaBar.readRegister(Rorc::RFID);
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

} // namespace roc
} // namespace AliceO2

