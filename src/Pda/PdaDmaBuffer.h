/// \file PdaDmaBuffer.h
/// \brief Definition of the PdaDmaBuffer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <vector>
#include <pda.h>

namespace AliceO2 {
namespace Rorc {
namespace Pda {

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

    const ScatterGatherVector& getScatterGatherList() const
    {
      return mScatterGatherVector;
    }

  private:
    DMABuffer* mDmaBuffer;
    PciDevice* mPciDevice;
    ScatterGatherVector mScatterGatherVector;
};

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
