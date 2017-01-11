/// \file ChannelMasterPdaBase.cxx
/// \brief Implementation of the ChannelMasterPdaBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelMasterPdaBase.h"
#include "Pda/Pda.h"
#include "Utilities/SmartPointer.h"

namespace AliceO2 {
namespace Rorc {

int ChannelMasterPdaBase::getSerialFromRorcDevice(const Parameters& parameters)
{
  auto id = parameters.getCardIdRequired();

  if (auto serial = boost::get<int>(&id)) {
    RorcDevice device {*serial};
    return device.getSerialNumber();
  } else if (auto address = boost::get<PciAddress>(&id)) {
    RorcDevice device {*address};
    return device.getSerialNumber();
  }
  BOOST_THROW_EXCEPTION(ParameterException()
      << ErrorInfo::Message("Either SerialNumber or PciAddress parameter required"));
}

ChannelMasterPdaBase::ChannelMasterPdaBase(CardType::type cardType, const Parameters& parameters,
    const AllowedChannels& allowedChannels, size_t fifoSize)
    : ChannelMasterBase(cardType, parameters, getSerialFromRorcDevice(parameters), allowedChannels), mDmaState(
        DmaState::STOPPED)
{
  auto paths = getPaths();

  // Initialize PDA & DMA objects
  Utilities::resetSmartPtr(mRorcDevice, getSerialNumber());
  Utilities::resetSmartPtr(mPdaBar, mRorcDevice->getPciDevice(), getChannelNumber());
  Utilities::resetSmartPtr(mMappedFilePages, paths.pages().string(),
      getChannelParameters().dma.bufferSize);
  Utilities::resetSmartPtr(mBufferPages, mRorcDevice->getPciDevice(), mMappedFilePages->getAddress(),
      mMappedFilePages->getSize(), getChannelNumber());
  partitionDmaBuffer(fifoSize, getChannelParameters().dma.pageSize);
}

ChannelMasterPdaBase::~ChannelMasterPdaBase()
{
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMasterPdaBase::startDma()
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    log("Unknown DMA state");
  } else if (mDmaState == DmaState::STARTED) {
    log("DMA already started. Ignoring startDma() call");
  } else {
    deviceStartDma();
  }
  mDmaState = DmaState::STARTED;
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMasterPdaBase::stopDma()
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    log("Unknown DMA state");
  } else if (mDmaState == DmaState::STOPPED) {
    log("Warning: DMA already stopped. Ignoring stopDma() call");
  } else {
    deviceStopDma();
  }
  mDmaState = DmaState::STOPPED;
}

void ChannelMasterPdaBase::resetChannel(ResetLevel::type resetLevel)
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Reset channel failed: DMA in unknown state"));
  }
  if (mDmaState != DmaState::STOPPED) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Reset channel failed: DMA was not stopped"));
  }

  deviceResetChannel(resetLevel);
}

uint32_t ChannelMasterPdaBase::readRegister(int index)
{
  return mPdaBar->getRegister<uint32_t>(index * sizeof(uint32_t));
}

void ChannelMasterPdaBase::writeRegister(int index, uint32_t value)
{
  mPdaBar->setRegister<uint32_t>(index * sizeof(uint32_t), value);
}

void ChannelMasterPdaBase::partitionDmaBuffer(size_t fifoSize, size_t pageSize)
{
  /// Amount of space reserved for the FIFO, we use multiples of the page size for uniformity
  size_t fifoSpace = ((fifoSize / pageSize) + 1) * pageSize;
  PageAddress fifoAddress;
  std::tie(fifoAddress, mPageAddresses) = Pda::partitionScatterGatherList(mBufferPages->getScatterGatherList(),
      fifoSpace, pageSize);
  mFifoAddressUser = const_cast<void*>(fifoAddress.user);
  mFifoAddressBus = const_cast<void*>(fifoAddress.bus);
}


} // namespace Rorc
} // namespace AliceO2
