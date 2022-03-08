
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
/// \file PdaDmaBuffer.cxx
/// \brief Implementation of the PdaDmaBuffer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <numeric>
#include "PdaDmaBuffer.h"
#include <pda.h>
#include "ExceptionInternal.h"
#include "Pda/PdaLock.h"
#include "ReadoutCard/InterprocessLock.h"
#include "ReadoutCard/Logger.h"

namespace o2
{
namespace roc
{
namespace Pda
{

PdaDmaBuffer::PdaDmaBuffer(PciDevice* pciDevice, void* userBufferAddress, size_t userBufferSize,
                           int dmaBufferId, SerialId serialId, bool requireHugepage) : mPciDevice(pciDevice)
{
  // Safeguard against PDA kernel module deadlocks, since it does not like parallel buffer registration
  try {
    Pda::PdaLock lock{};
  } catch (const LockException& e) {
    Logger::get() << "Failed to acquire PDA lock" << e.what() << LogErrorDevel << endm;
    throw;
  }

  try {
    // Tell PDA we're using our already allocated userspace buffer.
    if (PciDevice_registerDMABuffer(pciDevice, dmaBufferId, userBufferAddress, userBufferSize,
                                    &mDmaBuffer) != PDA_SUCCESS) {
      // Failed to register it. Usually, this means a DMA buffer wasn't cleaned up properly (such as after a crash).
      // So, try to clean things up.

      // Get the previous buffer
      DMABuffer* tempDmaBuffer;
      if (PciDevice_getDMABuffer(pciDevice, dmaBufferId, &tempDmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(PdaException() << ErrorInfo::Message(
                                "Failed to register external DMA buffer; Failed to get previous buffer for cleanup"));
      }

      // Free it
      if (PciDevice_deleteDMABuffer(pciDevice, tempDmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(PdaException() << ErrorInfo::Message(
                                "Failed to register external DMA buffer; Failed to delete previous buffer for cleanup"));
      }

      // Retry the registration of our new buffer
      if (PciDevice_registerDMABuffer(pciDevice, dmaBufferId, userBufferAddress, userBufferSize,
                                      &mDmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(PdaException() << ErrorInfo::Message(
                                "Failed to register external DMA buffer; Failed retry after automatic cleanup of previous buffer"));
      }
    }
  } catch (PdaException& e) {
    addPossibleCauses(e, { "Program previously exited without cleaning up DMA buffer, reinserting DMA kernel module may "
                           " help, but ensure no channels are open before reinsertion (modprobe -r uio_pci_dma; modprobe uio_pci_dma" });
    throw;
  }

  try {
    DMABuffer_SGNode* sgList;
    if (DMABuffer_getSGList(mDmaBuffer, &sgList) != PDA_SUCCESS) {
      BOOST_THROW_EXCEPTION(PdaException() << ErrorInfo::Message("Failed to get scatter-gather list"));
    }

    auto node = sgList;
    std::vector<size_t> nodeSizes;
    while (node != nullptr) {
      nodeSizes.emplace_back(node->length);
      size_t hugePageMinSize = 1024 * 1024 * 2; // 2 MiB, the smallest hugepage size
      if (requireHugepage && node->length < hugePageMinSize) {
        BOOST_THROW_EXCEPTION(
          PdaException() << ErrorInfo::Message("SGL node smaller than 2 MiB. IOMMU off and buffer not backed by hugapages - unsupported buffer configuration"));
      }

      ScatterGatherEntry e;
      e.size = node->length;
      e.addressUser = reinterpret_cast<uintptr_t>(node->u_pointer);
      e.addressBus = reinterpret_cast<uintptr_t>(node->d_pointer);
      e.addressKernel = reinterpret_cast<uintptr_t>(node->k_pointer);
      mScatterGatherVector.push_back(e);
      node = node->next;
    }

    if (mScatterGatherVector.empty()) {
      BOOST_THROW_EXCEPTION(PdaException() << ErrorInfo::Message(
                              "Failed to initialize scatter-gather list, was empty"));
    }

    // Print some stats regarding the Scatter-Gather list
    std::sort(nodeSizes.begin(), nodeSizes.end());
    int n = nodeSizes.size();
    int minSize = nodeSizes[0];
    int maxSize = nodeSizes[n - 1];
    double median = nodeSizes[n / 2];
    if (n % 2 == 0) {
      median = (median + nodeSizes[n / 2 - 1]) / 2;
    }
    int totalSize = std::accumulate(nodeSizes.begin(), nodeSizes.end(), 0);

    Logger::get() << "[" << serialId << " |"
                  << " PDA buffer SGL stats] #nodes: " << n << " | total: " << totalSize << " | min: " << minSize << " | max: " << maxSize << " | median: " << median << LogInfoDevel << endm;
  } catch (const PdaException&) {
    PciDevice_deleteDMABuffer(mPciDevice, mDmaBuffer);
    throw;
  }
}

PdaDmaBuffer::~PdaDmaBuffer()
{
  // Safeguard against PDA kernel module deadlocks, since it does not like parallel buffer registration
  // NOTE: not sure if necessary for deregistration as well
  try {
    Pda::PdaLock lock{};
  } catch (const LockException& e) {
    Logger::get() << "Failed to acquire PDA lock" << e.what() << LogErrorDevel << endm;
    assert(false);
    //throw; (changed to assert(false) to get rid of the warning)
  }

  try {
    PciDevice_deleteDMABuffer(mPciDevice, mDmaBuffer);
  } catch (std::exception& e) {
    // Nothing to be done?
    Logger::get() << "PdaDmaBuffer::~PdaDmaBuffer() failed: " << e.what() << LogErrorDevel << endm;
  }
}

uintptr_t PdaDmaBuffer::getBusOffsetAddress(size_t offset) const
{
  const auto& list = mScatterGatherVector;

  auto userBase = list.at(0).addressUser;
  auto userWithOffset = userBase + offset;

  // First we find the SGL entry that contains our address
  for (size_t i = 0; i < list.size(); ++i) {
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

} // namespace Pda
} // namespace roc
} // namespace o2
