///
/// \file BarWrapper.cxx
/// \author Pascal Boeschoten
///

#include "PdaDmaBuffer.h"
#include <pda.h>
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {


PdaDmaBuffer::PdaDmaBuffer(PciDevice* pciDevice, void* userBufferAddress, size_t userBufferSize, int dmaBufferId)
    : pciDevice(pciDevice)
{
  // Tell PDA we're using our already allocated userspace buffer.
  if (PciDevice_registerDMABuffer(pciDevice, dmaBufferId, userBufferAddress, userBufferSize, &dmaBuffer)
      != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to register external DMA buffer");
  }

  DMABuffer_SGNode* sgList;
  if (DMABuffer_getSGList(dmaBuffer, &sgList) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get scatter-gather list");
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
    ALICEO2_RORC_THROW_EXCEPTION("Failed to initialize scatter-gather list, was empty");
  }
}

PdaDmaBuffer::~PdaDmaBuffer()
{
  PciDevice_deleteDMABuffer(pciDevice, dmaBuffer);
}

} // namespace Rorc
} // namespace AliceO2
