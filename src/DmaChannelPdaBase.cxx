/// \file DmaChannelPdaBase.cxx
/// \brief Implementation of the DmaChannelPdaBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DmaChannelPdaBase.h"
#include <boost/filesystem/path.hpp>
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

DmaChannelPdaBase::DmaChannelPdaBase(const Parameters& parameters,
    const AllowedChannels& allowedChannels)
    : DmaChannelBase(getDescriptor(parameters), parameters, allowedChannels), mDmaState(DmaState::STOPPED)
{
  // Initialize PDA & DMA objects
  Utilities::resetSmartPtr(mRocPciDevice, getSerialNumber());

  // Register user's page data buffer
  log("Initializing memory-mapped DMA buffer", InfoLogger::InfoLogger::Debug);
  Utilities::resetSmartPtr(mPdaDmaBuffer, mRocPciDevice->getPciDevice(), getBufferProvider().getAddress(),
      getBufferProvider().getSize(), getPdaDmaBufferIndexPages(getChannelNumber(), 0));
  log(std::string("Scatter-gather list size: ") + std::to_string(mPdaDmaBuffer->getScatterGatherList().size()));
}

DmaChannelPdaBase::~DmaChannelPdaBase()
{
}

// Checks DMA state and forwards call to subclass if necessary
void DmaChannelPdaBase::startDma()
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
void DmaChannelPdaBase::stopDma()
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

void DmaChannelPdaBase::resetChannel(ResetLevel::type resetLevel)
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

uintptr_t DmaChannelPdaBase::getBusOffsetAddress(size_t offset)
{
  return getPdaDmaBuffer().getBusOffsetAddress(offset);
}

void DmaChannelPdaBase::checkSuperpage(const Superpage& superpage)
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
