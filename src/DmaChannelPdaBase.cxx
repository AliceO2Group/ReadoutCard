
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
/// \file DmaChannelPdaBase.cxx
/// \brief Implementation of the DmaChannelPdaBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DmaChannelPdaBase.h"
#include <boost/filesystem/path.hpp>
#include "Common/Iommu.h"
#include "Utilities/MemoryMaps.h"
#include "Utilities/Numa.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/Util.h"
#include "DmaBufferProvider/PdaDmaBufferProvider.h"
#include "DmaBufferProvider/FilePdaDmaBufferProvider.h"
#include "DmaBufferProvider/NullDmaBufferProvider.h"
#include "ReadoutCard/ParameterTypes/SerialId.h"
#include "Visitor.h"

namespace o2
{
namespace roc
{
namespace
{

CardDescriptor createCardDescriptor(const Parameters& parameters)
{
  return RocPciDevice(parameters.getCardIdRequired()).getCardDescriptor();
}

} // namespace

DmaChannelPdaBase::DmaChannelPdaBase(const Parameters& parameters,
                                     const AllowedChannels& allowedChannels)
  : DmaChannelBase(createCardDescriptor(parameters), const_cast<Parameters&>(parameters), allowedChannels), mDmaState(DmaState::STOPPED)
{
  // Initialize PDA & DMA objects
  Utilities::resetSmartPtr(mRocPciDevice, getCardDescriptor().pciAddress);

  // Create/register buffer
  if (auto bufferParameters = parameters.getBufferParameters()) {
    // Create appropriate BufferProvider subclass
    auto bufferId = getPdaDmaBufferIndexPages(getChannelNumber(), 0);
    mBufferProvider = Visitor::apply<std::unique_ptr<DmaBufferProviderInterface>>(
      *bufferParameters,
      [&](buffer_parameters::Memory parameters) {
        log("Initializing with DMA buffer from memory region", LogDebugDevel_(4205));
        return std::make_unique<PdaDmaBufferProvider>(mRocPciDevice->getPciDevice(), parameters.address,
                                                      parameters.size, bufferId, mRocPciDevice->getSerialId(), true);
      },
      [&](buffer_parameters::File parameters) {
        log("Initializing with DMA buffer from memory-mapped file", LogDebugDevel_(4206));
        return std::make_unique<FilePdaDmaBufferProvider>(mRocPciDevice->getPciDevice(), parameters.path,
                                                          parameters.size, bufferId, mRocPciDevice->getSerialId(), true);
      },
      [&](buffer_parameters::Null) {
        log("Initializing with null DMA buffer", LogDebugDevel_(4207));
        return std::make_unique<NullDmaBufferProvider>();
      });
    //TODO: This can be simplified as only the
    //first case is used...
  } else {
    BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("DmaChannel requires buffer_parameters"));
  }

  // Check if scatter-gather list is not suspicious
  {
    auto listSize = mBufferProvider->getScatterGatherListSize();
    auto hugePageMinSize = 1024 * 1024 * 2; // 2 MiB, the smallest hugepage size
    auto bufferSize = getBufferProvider().getSize();
    if (listSize > (bufferSize / hugePageMinSize)) {
      std::string message =
        "Scatter-gather list size greater than buffer size divided by 2MiB (minimum hugepage size)."
        " This means the IOMMU is off and the buffer is not backed by hugepages - an unsupported buffer configuration.";
      log(message, LogErrorDevel_(4208));
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(message));
    }
  }

  // Check memory mappings if it's hugepage
  if (getBufferProvider().getSize() > 0) {
    // Non-null buffer
    bool checked = false;
    // Get the memory mappings from linux
    const auto maps = Utilities::getMemoryMaps();
    const auto bufferAddress = reinterpret_cast<uintptr_t>(getBufferProvider().getAddress());
    for (const auto& map : maps) {
      // Match the map address with the provided buffer address
      if (map.addressStart == bufferAddress) {
        if (map.pageSizeKiB > 4) {
          log("Buffer is hugepage-backed", LogDebugTrace_(4209));
        } else {
          if (AliceO2::Common::Iommu::isEnabled()) {
            log("Buffer is NOT hugepage-backed, but IOMMU is enabled", LogDebugTrace_(4210));
          } else {
            std::string message =
              "Buffer is NOT hugepage-backed and IOMMU is disabled - unsupported buffer "
              "configuration";
            log(message, LogWarningDevel_(4211));
            BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(message)
                                              << ErrorInfo::PossibleCauses({ "roc-setup-hugetlbfs was not run" }));
          }
        }
        checked = true;
        break;
      }
    }
    if (!checked) {
      log("Failed to check if buffer is hugepage-backed", LogDebugTrace_(4212));
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
    log("Unknown DMA state", LogErrorDevel_(4213));
  } else if (mDmaState == DmaState::STARTED) {
    log("DMA already started. Ignoring startDma() call", LogWarningDevel_(4214));
  } else {
    log("Starting DMA", LogInfoDevel_(4215));
    deviceStartDma();
  }
  mDmaState = DmaState::STARTED;
}

// Checks DMA state and forwards call to subclass if necessary
void DmaChannelPdaBase::stopDma()
{
  if (mDmaState == DmaState::UNKNOWN) {
    log("Unknown DMA state", LogErrorDevel_(4216));
    mDmaState = DmaState::STOPPED;
  } else if (mDmaState == DmaState::STOPPED) {
    log("DMA already stopped. Ignoring stopDma() call", LogWarningDevel_(4217));
  } else {
    log("Stopping DMA", LogInfoDevel_(4218));
    mDmaState = DmaState::STOPPED;
    deviceStopDma();
  }
}

void DmaChannelPdaBase::resetChannel(ResetLevel::type resetLevel)
{
  if (mDmaState == DmaState::UNKNOWN) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Reset channel failed: DMA in unknown state"));
  }
  if (mDmaState != DmaState::STOPPED) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Reset channel failed: DMA was not stopped"));
  }

  log("Resetting channel", LogDebugDevel_(4219));
  deviceResetChannel(resetLevel);
}

uintptr_t DmaChannelPdaBase::getBusOffsetAddress(size_t offset)
{
  return getBufferProvider().getBusOffsetAddress(offset);
}

void DmaChannelPdaBase::checkSuperpage(const Superpage& superpage)
{
  if (superpage.getSize() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, size == 0"));
  }

  if (!Utilities::isMultiple(superpage.getSize(), size_t(32 * 1024))) {
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

PciAddress DmaChannelPdaBase::getPciAddress()
{
  return getCardDescriptor().pciAddress;
}

int DmaChannelPdaBase::getNumaNode()
{
  return Utilities::getNumaNode(getPciAddress());
}

} // namespace roc
} // namespace o2
