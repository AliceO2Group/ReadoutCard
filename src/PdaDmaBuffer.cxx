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
}

PdaDmaBuffer::~PdaDmaBuffer()
{
  PciDevice_deleteDMABuffer(pciDevice, dmaBuffer);
}

} // namespace Rorc
} // namespace AliceO2
