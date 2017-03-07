/// \file ChannelMasterPdaBase.cxx
/// \brief Implementation of the ChannelMasterPdaBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelMasterPdaBase.h"
#include "Pda/Pda.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/Util.h"
#include "Visitor.h"

namespace AliceO2 {
namespace Rorc {
namespace {

CardDescriptor getDescriptor(const Parameters& parameters)
{
  return Visitor::apply<CardDescriptor>(parameters.getCardIdRequired(),
      [&](int serial) {return RorcDevice(serial).getCardDescriptor();},
      [&](const PciAddress& address) {return RorcDevice(address).getCardDescriptor();});
}
}

ChannelMasterPdaBase::ChannelMasterPdaBase(CardType::type cardType, const Parameters& parameters,
    const AllowedChannels& allowedChannels, size_t fifoSize)
    : ChannelMasterBase(getDescriptor(parameters), parameters, allowedChannels), mDmaState(DmaState::STOPPED)
{
  auto paths = getPaths();

  // Initialize PDA & DMA objects
  Utilities::resetSmartPtr(mRorcDevice, getSerialNumber());

  log("Initializing BAR", InfoLogger::InfoLogger::Debug);
  Utilities::resetSmartPtr(mPdaBar, mRorcDevice->getPciDevice(), getChannelNumber());

  log("Initializing memory-mapped DMA buffer", InfoLogger::InfoLogger::Debug);
  if (getBufferProvider().getReservedSize() < fifoSize) {
    BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("Buffer reserved region too small"));
  }

  Utilities::resetSmartPtr(mPdaDmaBuffer, mRorcDevice->getPciDevice(), getBufferProvider().getBufferStartAddress(),
      getBufferProvider().getBufferSize(), getChannelNumber());

  mFifoAddressUser = getBufferProvider().getReservedStartAddress();
  mFifoAddressBus = getBusOffsetAddress(getBufferProvider().getReservedOffset());
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
    log("Starting DMA", InfoLogger::InfoLogger::Debug);
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
    log("Stopping DMA", InfoLogger::InfoLogger::Debug);
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

  log("Resetting channel", InfoLogger::InfoLogger::Debug);
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

uintptr_t ChannelMasterPdaBase::getBusOffsetAddress(size_t offset)
{
  const auto& list = getPdaDmaBuffer().getScatterGatherList();

  auto userBase = list.at(0).addressUser;
  auto userWithOffset = userBase + offset;

  // First we find the SGL entry that contains our address
  for (int i = 0; i < list.size(); ++i) {
    auto entryUserStartAddress = list[i].addressUser;
    auto entryUserEndAddress = entryUserStartAddress + list[i].size;

    if ((userWithOffset >= entryUserStartAddress) && (userWithOffset < entryUserEndAddress)) {
      // This is the entry we need
      // We now need to calculate the difference from the start of this entry to the given offset. We make use of the
      // fact that the userspace addresses will be contiguous
      auto entryOffset = userWithOffset - entryUserStartAddress;
      auto offsetBusAddress = list[i].addressBus + entryOffset;
      return offsetBusAddress;
    }
  }

  BOOST_THROW_EXCEPTION(Exception()
      << ErrorInfo::Message("Physical offset address out of range")
      << ErrorInfo::Offset(offset));
}



} // namespace Rorc
} // namespace AliceO2
