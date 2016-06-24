///
/// \file BarWrapper.h
/// \author Pascal Boeschoten
///

#pragma once

#include <pda.h>

namespace AliceO2 {
namespace Rorc {

/// Handles the creation and cleanup of a PDA DMABuffer object, registering a user-allocated buffer
class PdaDmaBuffer
{
  public:
    /// Construct the buffer wrapper
    /// \param pciDevice
    /// \param userBufferAddress Address of the user-allocated buffer
    /// \param userBufferSize Size of the user-allocated buffer
    /// \param dmaBufferId Unique ID to use for registering the buffer (uniqueness must be channel-wide, probably)
    PdaDmaBuffer(PciDevice* pciDevice, void* userBufferAddress, size_t userBufferSize, int dmaBufferId);
    ~PdaDmaBuffer();

  private:
    DMABuffer* dmaBuffer;
    PciDevice* pciDevice;
};

} // namespace Rorc
} // namespace AliceO2
