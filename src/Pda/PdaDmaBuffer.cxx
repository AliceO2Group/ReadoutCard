/// \file PdaDmaBuffer.cxx
/// \brief Implementation of the PdaDmaBuffer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "PdaDmaBuffer.h"
#include <pda.h>
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace Rorc {
namespace Pda {

PdaDmaBuffer::PdaDmaBuffer(PciDevice* pciDevice, void* userBufferAddress, size_t userBufferSize, int dmaBufferId)
    : mPciDevice(pciDevice)
{
  try {
    // Tell PDA we're using our already allocated userspace buffer.
    if (PciDevice_registerDMABuffer(pciDevice, dmaBufferId, userBufferAddress, userBufferSize, &mDmaBuffer)
        != PDA_SUCCESS) {
      // Failed to register it. Usually, this means a DMA buffer wasn't cleaned up properly (such as after a crash).
      // So, try to clean things up.

      // Get the previous buffer
      DMABuffer* tempDmaBuffer;
      if (PciDevice_getDMABuffer(pciDevice, dmaBufferId, &tempDmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(RorcPdaException() << ErrorInfo::Message(
            "Failed to register external DMA buffer; Failed to get previous buffer for cleanup"));
      }

      // Free it
      if (PciDevice_deleteDMABuffer(pciDevice, tempDmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(RorcPdaException() << ErrorInfo::Message(
            "Failed to register external DMA buffer; Failed to delete previous buffer for cleanup"));
      }

      // Retry the registration of our new buffer
      if(PciDevice_registerDMABuffer(pciDevice, dmaBufferId, userBufferAddress, userBufferSize, &mDmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(RorcPdaException() << ErrorInfo::Message(
            "Failed to register external DMA buffer; Failed retry after automatic cleanup of previous buffer"));
      }
    }
  }
  catch (RorcPdaException& e) {
    addPossibleCauses(e, {"Program previously exited without cleaning up DMA buffer, reinserting DMA kernel module may "
        " help, but ensure no channels are open before reinsertion (modprobe -r uio_pci_dma; modprobe uio_pci_dma"});
  }

  DMABuffer_SGNode* sgList;
  if (DMABuffer_getSGList(mDmaBuffer, &sgList) != PDA_SUCCESS) {
    BOOST_THROW_EXCEPTION(RorcPdaException() << ErrorInfo::Message("Failed to get scatter-gather list"));
  }

  auto node = sgList;

  while (node != nullptr) {
    ScatterGatherEntry e;
    e.size = node->length;
    e.addressUser = node->u_pointer;
    e.addressBus = node->d_pointer;
    e.addressKernel = node->k_pointer;
    mScatterGatherVector.push_back(e);
    node = node->next;
  }

  if (mScatterGatherVector.empty()) {
    BOOST_THROW_EXCEPTION(RorcPdaException() << ErrorInfo::Message(
        "Failed to initialize scatter-gather list, was empty"));
  }
}

PdaDmaBuffer::~PdaDmaBuffer()
{
  PciDevice_deleteDMABuffer(mPciDevice, mDmaBuffer);
}

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
