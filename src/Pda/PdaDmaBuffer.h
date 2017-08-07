/// \file PdaDmaBuffer.h
/// \brief Definition of the PdaDmaBuffer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <vector>
#include <pda.h>
#include "Pda/PdaDevice.h"

namespace AliceO2 {
namespace roc {
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
    /// \param dmaBufferId Unique ID to use for registering the buffer (uniqueness must be card-wide)
    /// \param requireHugepage Require the buffer to have hugepage-sized scatter-gather list nodes
    PdaDmaBuffer(PdaDevice::PdaPciDevice pciDevice, void* userBufferAddress, size_t userBufferSize,
        int dmaBufferId, bool requireHugepage = true);

    ~PdaDmaBuffer();

    struct ScatterGatherEntry
    {
      size_t size;
      uintptr_t addressUser;
      uintptr_t addressBus;
      uintptr_t addressKernel;
    };
    using ScatterGatherVector = std::vector<ScatterGatherEntry>;

    const ScatterGatherVector& getScatterGatherList() const
    {
      return mScatterGatherVector;
    }

    /// Function for getting the bus address that corresponds to the user address + given offset
    uintptr_t getBusOffsetAddress(size_t offset) const;

  private:
    DMABuffer* mDmaBuffer;
    PdaDevice::PdaPciDevice mPciDevice;
    ScatterGatherVector mScatterGatherVector;
};

} // namespace Pda
} // namespace roc
} // namespace AliceO2
