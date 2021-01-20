// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CrorcDmaChannel.cxx
/// \brief Implementation of the CrorcDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

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
#include "ReadoutCard/ChannelFactory.h"
#include "Utilities/SmartPointer.h"

namespace b = boost;
//namespace bip = boost::interprocess;
//namespace bfs = boost::filesystem;
using std::cout;
using std::endl;
using namespace std::literals;

namespace AliceO2
{
namespace roc
{

CrorcDmaChannel::CrorcDmaChannel(const Parameters& parameters)
  : DmaChannelPdaBase(parameters, allowedChannels()),
    mPageSize(parameters.getDmaPageSize().get_value_or(DMA_PAGE_SIZE)), // 8 kB default for uniformity with CRU
    mSTBRD(parameters.getStbrdEnabled().get_value_or(false)),
    mUseFeeAddress(false),                                                     // Not sure
    mDataSource(parameters.getDataSource().get_value_or(DataSource::Internal)) // Internal loopback by default
{
  // Check that the DMA page is valid
  if (mPageSize != DMA_PAGE_SIZE) {
    BOOST_THROW_EXCEPTION(CrorcException() << ErrorInfo::Message("CRORC only supports 8KiB DMA page size")
                                           << ErrorInfo::DmaPageSize(mPageSize));
  }

  // Check that the data source is valid. If not throw
  if (mDataSource == DataSource::Ddg) {
    BOOST_THROW_EXCEPTION(CruException() << ErrorInfo::Message("CRORC does not support specified data source")
                                         << ErrorInfo::DataSource(mDataSource));
  }

  mGeneratorEnabled = (mDataSource == DataSource::Fee) ? false : true;

  // Set mRDYRX if generator is disabled and mSTBRD is false
  if (!mGeneratorEnabled) {
    mRDYRX = mSTBRD ? false : true;
  }

  // Prep for BAR
  auto bar = ChannelFactory().getBar(parameters);
  crorcBar = std::move(std::dynamic_pointer_cast<CrorcBar>(bar)); // Initialize bar

  // Create and register our ReadyFIFO buffer
  log("Initializing ReadyFIFO DMA buffer", LogDebugDevel);
  {
    // Create and register the buffer
    // Note: if resizing the file fails, we might've accidentally put the file in a hugetlbfs mount with 1 GB page size
    constexpr auto FIFO_SIZE = sizeof(ReadyFifo);
    Utilities::resetSmartPtr(mBufferFifoFile, getPaths().fifo(), FIFO_SIZE, true);
    Utilities::resetSmartPtr(mPdaDmaBufferFifo, getRocPciDevice().getPciDevice(), mBufferFifoFile->getAddress(),
                             FIFO_SIZE, getPdaDmaBufferIndexFifo(getChannelNumber()), false); // note the 'false' at the end specifies non-hugepage memory

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

  if (mDataSource == DataSource::Fee || mDataSource == DataSource::Siu) {
    deviceResetChannel(ResetLevel::InternalSiu);
  } else {
    deviceResetChannel(ResetLevel::Internal);
  }
}

auto CrorcDmaChannel::allowedChannels() -> AllowedChannels
{
  return { 0, 1, 2, 3, 4, 5 };
}

CrorcDmaChannel::~CrorcDmaChannel()
{
}

void CrorcDmaChannel::deviceStartDma()
{
  deviceResetChannel(ResetLevel::Internal);

  startDataReceiving();

  log("DMA start deferred until enough superpages available", LogInfoDevel);

  mFreeFifoFront = 0;
  mFreeFifoBack = 0;
  mFreeFifoSize = 0;
  while (!mReadyQueue.isEmpty()) {
    mReadyQueue.popFront();
  }
  while (!mTransferQueue.isEmpty()) {
    mTransferQueue.popFront();
  }
  mPendingDmaStart = true;
}

void CrorcDmaChannel::startPendingDma()
{
  if (!mPendingDmaStart) {
    return;
  }

  if (mTransferQueue.isEmpty()) { // We should never end up in here
    log("Insufficient superpages to start pending DMA", LogErrorDevel);
    return;
  }

  log("Starting pending DMA", LogInfoDevel);

  if (mGeneratorEnabled) {
    log("Starting data generator", LogInfoDevel);
    //getBar()->startDataGenerator();
    startDataGenerator();
  } else {
    if (mRDYRX || mSTBRD) {
      log("Starting trigger", LogInfoDevel);

      // Clearing SIU/DIU status.
      getBar()->assertLinkUp();

      // RDYRX command to FEE
      uint32_t command = (mRDYRX) ? Crorc::Registers::RDYRX : Crorc::Registers::STBRD;
      getBar()->startTrigger(command);
    }
  }

  std::this_thread::sleep_for(100ms);

  mPendingDmaStart = false;
  log("DMA started", LogInfoOps);
}

void CrorcDmaChannel::deviceStopDma()
{
  getBar()->flushSuperpages();
  if (mGeneratorEnabled) {
    getBar()->stopDataGenerator();
  } else {
    if (mRDYRX || mSTBRD) {
      // Sending EOBTR to FEE.
      getBar()->stopTrigger();
    }
  }
  getBar()->stopDataReceiver();

  // Return any filled superpages
  fillSuperpages();

  // Return any superpages that have been pushed up in the meantime but won't get filled
  while (mTransferQueue.sizeGuess()) {
    auto superpage = mTransferQueue.frontPtr();
    superpage->setReceived(0);
    superpage->setReady(false);
    mReadyQueue.write(*superpage);
    mTransferQueue.popFront();
  }
}

void CrorcDmaChannel::deviceResetChannel(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::Nothing) {
    return;
  }

  bool resetSiu = (resetLevel == ResetLevel::InternalSiu) ? true : false;
  getBar()->resetDevice(resetSiu);
}

void CrorcDmaChannel::startDataGenerator() //TODO: Update this
{
  if (DataSource::Internal == mDataSource) {
    getBar()->setLoopback(); //TODO: Is there a way to test this?
  }

  if (DataSource::Diu == mDataSource) {
    getBar()->setDiuLoopback();
  }

  if (DataSource::Siu == mDataSource) {
    getBar()->setSiuLoopback();
  }

  getBar()->startDataGenerator();
}

void CrorcDmaChannel::startDataReceiving()
{
  getBar()->startDataReceiver(mReadyFifoAddressBus);
}

int CrorcDmaChannel::getTransferQueueAvailable()
{
  return TRANSFER_QUEUE_CAPACITY - mTransferQueue.sizeGuess();
}

int CrorcDmaChannel::getReadyQueueSize()
{
  return mReadyQueue.sizeGuess();
}

auto CrorcDmaChannel::getSuperpage() -> Superpage
{
  if (mReadyQueue.isEmpty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get superpage, ready queue was empty"));
  }
  return *mReadyQueue.frontPtr();
}

bool CrorcDmaChannel::pushSuperpage(Superpage superpage)
{
  if (mDmaState != DmaState::STARTED) {
    return false;
  }

  checkSuperpage(superpage);

  if (mTransferQueue.sizeGuess() >= TRANSFER_QUEUE_CAPACITY) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
  }

  if (mFreeFifoSize >= MAX_SUPERPAGE_DESCRIPTORS) {
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Could not push superpage, firmware queue was full (this should never happen)"));
  }

  auto busAddress = getBusOffsetAddress(superpage.getOffset());
  pushFreeFifoPage(mFreeFifoFront, busAddress, superpage.getSize());
  mFreeFifoSize++;
  mFreeFifoFront = (mFreeFifoFront + 1) % MAX_SUPERPAGE_DESCRIPTORS;

  mTransferQueue.write(superpage);

  return true;
}

auto CrorcDmaChannel::popSuperpage() -> Superpage
{
  if (mReadyQueue.isEmpty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, ready queue was empty"));
  }
  auto superpage = mReadyQueue.frontPtr();
  mReadyQueue.popFront();
  return *superpage;
}

void CrorcDmaChannel::fillSuperpages()
{
  if (mPendingDmaStart) {
    if (!mTransferQueue.isEmpty()) {
      startPendingDma();
    } else {
      // Waiting on enough superpages to start DMA...
      return;
    }
  }

  // Check for arrivals & handle them
  if (!mTransferQueue.isEmpty()) { // i.e. If something is pushed to the CRORC
    auto isArrived = [&](int descriptorIndex) { return dataArrived(descriptorIndex) == DataArrivalStatus::WholeArrived; };
    auto resetDescriptor = [&](int descriptorIndex) { getReadyFifoUser()->entries[descriptorIndex].reset(); };
    auto getLength = [&](int descriptorIndex) { return getReadyFifoUser()->entries[descriptorIndex].length * 4; }; // length in 4B words

    while (mFreeFifoSize > 0) {
      if (isArrived(mFreeFifoBack)) {
        //size_t superpageFilled = SUPERPAGE_SIZE; // Get the length before updating our descriptor index
        size_t superpageFilled = getLength(mFreeFifoBack); // Get the length before updating our descriptor index
        resetDescriptor(mFreeFifoBack);

        mFreeFifoSize--;
        mFreeFifoBack = (mFreeFifoBack + 1) % MAX_SUPERPAGE_DESCRIPTORS;

        // Push Superpage
        auto superpage = mTransferQueue.frontPtr();
        superpage->setReceived(superpageFilled);
        superpage->setReady(true);
        mReadyQueue.write(*superpage);
        mTransferQueue.popFront();
      } else {
        // If the back one hasn't arrived yet, the next ones will certainly not have arrived either...
        break;
      }
    }
  }
}

// Return a boolean that denotes whether the transfer queue is empty
// The transfer queue is empty when all its slots are available
bool CrorcDmaChannel::isTransferQueueEmpty()
{
  return mTransferQueue.isEmpty();
}

// Return a boolean that denotes whether the ready queue is full
// The ready queue is full when the CRORC has filled it up
bool CrorcDmaChannel::isReadyQueueFull()
{
  return mReadyQueue.sizeGuess() == READY_QUEUE_CAPACITY;
}

int32_t CrorcDmaChannel::getDroppedPackets()
{
  //log("No support for dropped packets in CRORC yet", InfoLogger::InfoLogger::Warning);
  return -1;
}

void CrorcDmaChannel::pushFreeFifoPage(int readyFifoIndex, uintptr_t pageBusAddress, int pageSize)
{
  size_t pageWords = pageSize / 4; // Size in 32-bit words
  getBar()->pushRxFreeFifo(pageBusAddress, pageWords, readyFifoIndex);
}

CrorcDmaChannel::DataArrivalStatus::type CrorcDmaChannel::dataArrived(int index)
{
  auto length = getReadyFifoUser()->entries[index].length;
  auto status = getReadyFifoUser()->entries[index].status;

  if (status == -1) {
    return DataArrivalStatus::NoneArrived;
  } else if (status == 0) {
    return DataArrivalStatus::PartArrived;
  } else if ((status & 0xff) == Crorc::Registers::DTSW) {
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
  return getBar()->getSerial();
}

boost::optional<std::string> CrorcDmaChannel::getFirmwareInfo()
{
  return getBar()->getFirmwareInfo();
}

} // namespace roc
} // namespace AliceO2
