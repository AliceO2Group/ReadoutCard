///
/// \file PdaDmaBuffer.h
/// \author Pascal Boeschoten
///

#pragma once

#include <vector>
#include <pda.h>

namespace AliceO2 {
namespace Rorc {

/// Handles the creation and cleanup of a PDA DMABuffer object, registering a user-allocated buffer and converting
/// the scatter-gather list of the buffer into a convenient vector format
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

    struct ScatterGatherEntry
    {
      size_t size;
      void* addressUser;
      void* addressBus;
      void* addressKernel;
    };
    using ScatterGatherVector = std::vector<ScatterGatherEntry>;

    inline const ScatterGatherVector& getScatterGatherList() const
    {
      return sgVector;
    }

  private:
    DMABuffer* dmaBuffer;
    PciDevice* pciDevice;
    ScatterGatherVector sgVector;
};

} // namespace Rorc
} // namespace AliceO2
