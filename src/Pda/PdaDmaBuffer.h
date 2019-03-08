// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file PdaDmaBuffer.h
/// \brief Definition of the PdaDmaBuffer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_PDA_PDADMABUFFER_H_
#define ALICEO2_SRC_READOUTCARD_PDA_PDADMABUFFER_H_

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

#endif // ALICEO2_SRC_READOUTCARD_PDA_PDADMABUFFER_H_
