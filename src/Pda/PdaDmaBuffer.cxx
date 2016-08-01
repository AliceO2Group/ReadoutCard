///
/// \file PdaDmaBuffer.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "PdaDmaBuffer.h"
#include <pda.h>
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {


PdaDmaBuffer::PdaDmaBuffer(PciDevice* pciDevice, void* userBufferAddress, size_t userBufferSize, int dmaBufferId)
    : pciDevice(pciDevice)
{
  try {
    // Tell PDA we're using our already allocated userspace buffer.
    if (PciDevice_registerDMABuffer(pciDevice, dmaBufferId, userBufferAddress, userBufferSize, &dmaBuffer)
        != PDA_SUCCESS) {
      // Failed to register it. Usually, this means a DMA buffer wasn't cleaned up properly (such as after a crash).
      // So, try to clean things up.

      // Get the previous buffer
      DMABuffer* tempDmaBuffer;
      if (PciDevice_getDMABuffer(pciDevice, dmaBufferId, &tempDmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(RorcPdaException() << errinfo_rorc_generic_message(
            "Failed to register external DMA buffer; Failed to get previous buffer for cleanup"));
      }

      // Free it
      if (PciDevice_deleteDMABuffer(pciDevice, tempDmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(RorcPdaException() << errinfo_rorc_generic_message(
            "Failed to register external DMA buffer; Failed to delete previous buffer for cleanup"));
      }

      // Retry the registration of our new buffer
      if(PciDevice_registerDMABuffer(pciDevice, dmaBufferId, userBufferAddress, userBufferSize, &dmaBuffer) != PDA_SUCCESS) {
        // Give up
        BOOST_THROW_EXCEPTION(RorcPdaException() << errinfo_rorc_generic_message(
            "Failed to register external DMA buffer; Failed retry after automatic cleanup of previous buffer"));
      }
    }
  } catch (RorcPdaException& e) {
    addPossibleCauses(e, {"Program previously exited without cleaning up DMA buffer, reinserting DMA kernel module may "
        " help, but insure no channels are open before reinsertion (modprobe -r uio_pci_dma; modprobe uio_pci_dma"});
  }

  DMABuffer_SGNode* sgList;
  if (DMABuffer_getSGList(dmaBuffer, &sgList) != PDA_SUCCESS) {
    BOOST_THROW_EXCEPTION(RorcPdaException() << errinfo_rorc_generic_message("Failed to get scatter-gather list"));
  }

  auto node = sgList;

  while (node != nullptr) {
    ScatterGatherEntry e;
    e.size = node->length;
    e.addressUser = node->u_pointer;
    e.addressBus = node->d_pointer;
    e.addressKernel = node->k_pointer;
    sgVector.push_back(e);
    node = node->next;
  }

  if (sgVector.empty()) {
    BOOST_THROW_EXCEPTION(RorcPdaException() << errinfo_rorc_generic_message(
        "Failed to initialize scatter-gather list, was empty"));
  }
}

PdaDmaBuffer::~PdaDmaBuffer()
{
  PciDevice_deleteDMABuffer(pciDevice, dmaBuffer);
}

} // namespace Rorc
} // namespace AliceO2
