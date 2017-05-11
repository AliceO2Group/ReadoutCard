/// \file ChannelMasterPdaBase.cxx
/// \brief Implementation of the ChannelMasterPdaBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelMasterPdaBase.h"
#include <boost/filesystem/path.hpp>
#include "Pda/Pda.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/Util.h"
#include "Visitor.h"

namespace AliceO2 {
namespace roc {
namespace {

CardDescriptor getDescriptor(const Parameters& parameters)
{
  return Visitor::apply<CardDescriptor>(parameters.getCardIdRequired(),
      [&](int serial) {return RocPciDevice(serial).getCardDescriptor();},
      [&](const PciAddress& address) {return RocPciDevice(address).getCardDescriptor();});
}

}

ChannelMasterPdaBase::ChannelMasterPdaBase(const Parameters& parameters,
    const AllowedChannels& allowedChannels)
    : ChannelMasterBase(getDescriptor(parameters), parameters, allowedChannels), mDmaState(DmaState::STOPPED)
{
  // Initialize PDA & DMA objects
  Utilities::resetSmartPtr(mRocPciDevice, getSerialNumber());

  log("Initializing BAR", InfoLogger::InfoLogger::Debug);
  Utilities::resetSmartPtr(mPdaBar, mRocPciDevice->getPciDevice(), getChannelNumber());

  // Register user's page data buffer
  log("Initializing memory-mapped DMA buffer", InfoLogger::InfoLogger::Debug);
  Utilities::resetSmartPtr(mPdaDmaBuffer, mRocPciDevice->getPciDevice(), getBufferProvider().getAddress(),
      getBufferProvider().getSize(), getPdaDmaBufferIndexPages(getChannelNumber(), 0));
  log(std::string("Scatter-gather list size: ") + std::to_string(mPdaDmaBuffer->getScatterGatherList().size()));
}

ChannelMasterPdaBase::~ChannelMasterPdaBase()
{
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMasterPdaBase::startDma()
{
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
  return mPdaBar->readRegister(index);
}

void ChannelMasterPdaBase::writeRegister(int index, uint32_t value)
{
  mPdaBar->writeRegister(index, value);
}

uintptr_t ChannelMasterPdaBase::getBusOffsetAddress(size_t offset)
{
  return getPdaDmaBuffer().getBusOffsetAddress(offset);
}

void ChannelMasterPdaBase::checkSuperpage(const Superpage& superpage)
{
  if (superpage.getSize() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, size == 0"));
  }

  if (!Utilities::isMultiple(superpage.getSize(), size_t(32*1024))) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Could not enqueue superpage, size not a multiple of 32 KiB"));
  }

  if ((superpage.getOffset() + superpage.getSize()) > getBufferProvider().getSize()) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Superpage out of range"));
  }

  if ((superpage.getOffset() % 4) != 0) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Superpage offset not 32-bit aligned"));
  }
}

} // namespace roc
} // namespace AliceO2
