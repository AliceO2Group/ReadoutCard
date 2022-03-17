
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
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
#include "Crorc/Constants.h"
#include "ReadoutCard/ChannelFactory.h"

using namespace std::literals;

namespace o2
{
namespace roc
{

CrorcDmaChannel::CrorcDmaChannel(const Parameters& parameters)
  : DmaChannelPdaBase(parameters, allowedChannels()),
    mPageSize(parameters.getDmaPageSize().get_value_or(DMA_PAGE_SIZE)), // 8 kB default for uniformity with CRU
    mSTBRD(parameters.getStbrdEnabled().get_value_or(false)),
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

  // Create and register our Superpage info (size + count) buffer
  log("Initializing Superpage info buffer", LogDebugDevel);
  {
    // Create and register the buffer
    // Note: if resizing the file fails, we might've accidentally put the file in a hugetlbfs mount with 1 GB page size
    SerialId serialId = { getSerialNumber(), getEndpointNumber() };
    Utilities::resetSmartPtr(mSuperpageInfoFile, getPaths().spInfo(), kSuperpageInfoSize, false);
    Utilities::resetSmartPtr(mPdaDmaBufferFifo, getRocPciDevice().getPciDevice(), mSuperpageInfoFile->getAddress(),
                             kSuperpageInfoSize, getPdaDmaBufferIndexFifo(getChannelNumber()), serialId, false); // note the 'false' at the end specifies non-hugepage memory

    const auto& entry = mPdaDmaBufferFifo->getScatterGatherList().at(0);
    if (entry.size < kSuperpageInfoSize) {
      // Something must've failed at some point
      BOOST_THROW_EXCEPTION(Exception()
                            << ErrorInfo::Message("Scatter gather list entry for Superpage info buffer was too small")
                            << ErrorInfo::ScatterGatherEntrySize(entry.size)
                            << ErrorInfo::SuperpageInfoSize(kSuperpageInfoSize));
    }
    mSuperpageInfoAddressUser = entry.addressUser;
    mSuperpageInfoAddressBus = entry.addressBus;
  }

  // Start this from 0xff, as the first valid count to be written will be 0x0
  getSuperpageInfoUser()->count = 0xff;

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

  while (!mReadyQueue.isEmpty()) {
    mReadyQueue.popFront();
  }
  while (!mTransferQueue.isEmpty()) {
    mTransferQueue.popFront();
  }

  if (mGeneratorEnabled) {
    log("Starting data generator", LogInfoDevel);
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
  getBar()->startDataReceiver(mSuperpageInfoAddressBus);
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

bool CrorcDmaChannel::isASuperpageAvailable()
{
  uint32_t newCount = getSuperpageInfoUser()->count;
  uint32_t diff;

  if (newCount < mSPAvailCount) { // handle overflow
    diff = ((0xff + 1) - 0xff) + newCount;
  } else {
    diff = newCount - mSPAvailCount;
  }
  mSPAvailCount = newCount;

  return diff > 0;
}

void CrorcDmaChannel::fillSuperpages()
{
  // Check for arrivals & handle them
  if (!mIntermediateQueue.isEmpty() && isASuperpageAvailable()) {

    auto superpage = mIntermediateQueue.frontPtr();
    superpage->setReceived(getSuperpageInfoUser()->size); // length in bytes
    superpage->setReady(true);
    mReadyQueue.write(*superpage);
    mIntermediateQueue.popFront();
  }

  // Push single Superpage to the firmware when available
  if (mIntermediateQueue.isEmpty() && !mTransferQueue.isEmpty()) {
    auto inSuperpage = mTransferQueue.frontPtr();
    mTransferQueue.popFront();

    auto busAddress = getBusOffsetAddress(inSuperpage->getOffset());
    getBar()->pushSuperpageAddressAndSize(busAddress, inSuperpage->getSize());

    mIntermediateQueue.write(*inSuperpage);
  }
}

// Return a boolean that denotes whether the transfer queue is empty
// The transfer queue is empty when all its slots are availabl//e
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

bool CrorcDmaChannel::areSuperpageFifosHealthy()
{
  return true;
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
} // namespace o2
