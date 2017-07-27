/// \file DmaChannelPdaBase.cxx
/// \brief Implementation of the DmaChannelPdaBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DmaChannelPdaBase.h"
#include <boost/filesystem/path.hpp>
#include "Common/Iommu.h"
#include "Utilities/MemoryMaps.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/Util.h"
#include "Visitor.h"

namespace AliceO2 {
namespace roc {
namespace {

CardDescriptor createCardDescriptor(const Parameters& parameters)
{
  return Visitor::apply<CardDescriptor>(parameters.getCardIdRequired(),
      [&](int serial) {return RocPciDevice(serial).getCardDescriptor();},
      [&](const PciAddress& address) {return RocPciDevice(address).getCardDescriptor();});
}

}

DmaChannelPdaBase::DmaChannelPdaBase(const Parameters& parameters,
    const AllowedChannels& allowedChannels)
    : DmaChannelBase(createCardDescriptor(parameters), parameters, allowedChannels), mDmaState(DmaState::STOPPED)
{
  // Initialize PDA & DMA objects
  Utilities::resetSmartPtr(mRocPciDevice, getCardDescriptor().pciAddress);

  // Register user's page data buffer
  log("Initializing memory-mapped DMA buffer", InfoLogger::InfoLogger::Debug);
  Utilities::resetSmartPtr(mPdaDmaBuffer, mRocPciDevice->getPciDevice(), getBufferProvider().getAddress(),
      getBufferProvider().getSize(), getPdaDmaBufferIndexPages(getChannelNumber(), 0));

  // Check if scatter-gather list is not suspicious
  {
    auto listSize = mPdaDmaBuffer->getScatterGatherList().size();
    auto hugePageMinSize = 1024*1024*2; // 2 MiB, the smallest hugepage size
    auto bufferSize = getBufferProvider().getSize();
    log(std::string("Scatter-gather list size: ") + std::to_string(listSize));
    if (listSize > (bufferSize / hugePageMinSize)) {
      std::string message = "Scatter-gather list size greater than buffer size divided by 2MiB (minimum hugepage size)."
        " This means the IOMMU is off and the buffer is not backed by hugepages - an unsupported buffer configuration.";
      log(message, InfoLogger::InfoLogger::Error);
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(message));
    }
  }

  // Check memory mappings if it's hugepage
  {
    bool checked = false;
    const auto maps = Utilities::getMemoryMaps();
    for (const auto& map : maps) {
      const auto bufferAddress = reinterpret_cast<uintptr_t>(getBufferProvider().getAddress());
      if (map.addressStart == bufferAddress) {
        if (map.pageSizeKiB > 4) {
          log("Buffer is hugepage-backed", InfoLogger::InfoLogger::Info);
        } else {
          if (Common::Iommu::isEnabled()) {
            log("Buffer is NOT hugepage-backed, but IOMMU is enabled", InfoLogger::InfoLogger::Warning);
          } else {
            std::string message = "Buffer is NOT hugepage-backed and IOMMU is disabled - unsupported buffer "
              "configuration";
            log(message, InfoLogger::InfoLogger::Error);
            BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(message));
          }
        }
        checked = true;
        break;
      }
    }
    if (!checked) {
      log("Failed to check if buffer is hugepage-backed", InfoLogger::InfoLogger::Warning);
    }
  }
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
