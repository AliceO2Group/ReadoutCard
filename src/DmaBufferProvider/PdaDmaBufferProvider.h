// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file PdaDmaBufferProvider.h
/// \brief Definition of the PdaDmaBufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_DMABUFFERPROVIDER_PDADMABUFFERPROVIDER_H_
#define O2_READOUTCARD_SRC_DMABUFFERPROVIDER_PDADMABUFFERPROVIDER_H_

#include <cstddef>
#include <cstdint>
#include <vector>
#include "DmaBufferProvider/DmaBufferProviderInterface.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaDmaBuffer.h"

namespace o2
{
namespace roc
{

/// Implementation of the DmaBufferProviderInterface for in-memory DMA buffers registered with PDA
class PdaDmaBufferProvider : public DmaBufferProviderInterface
{
 public:
  PdaDmaBufferProvider(PciDevice* pciDevice, void* userBufferAddress, size_t userBufferSize,
                       int dmaBufferId, bool requireHugepage)
    : mAddress(userBufferAddress), mSize(userBufferSize), mPdaBuffer(pciDevice, userBufferAddress, userBufferSize, dmaBufferId, requireHugepage)
  {
  }

  virtual ~PdaDmaBufferProvider() = default;

  /// Get starting userspace address of the DMA buffer
  virtual uintptr_t getAddress() const
  {
    return reinterpret_cast<uintptr_t>(mAddress);
  }

  /// Get total size of the DMA buffer
  virtual size_t getSize() const
  {
    return mSize;
  }

  /// Amount of entries in the scatter-gather list
  virtual size_t getScatterGatherListSize() const
  {
    return mPdaBuffer.getScatterGatherList().size();
  }

  /// Get size of an entry of the scatter-gather list
  virtual size_t getScatterGatherEntrySize(int index) const
  {
    return mPdaBuffer.getScatterGatherList().at(index).size;
  }

  /// Get userspace address of an entry of the scatter-gather list
  virtual uintptr_t getScatterGatherEntryAddress(int index) const
  {
    return mPdaBuffer.getScatterGatherList().at(index).addressUser;
  }

  /// Function for getting the bus address that corresponds to the user address + given offset
  virtual uintptr_t getBusOffsetAddress(size_t offset) const
  {
    return mPdaBuffer.getBusOffsetAddress(offset);
  }

 private:
  void* mAddress;
  size_t mSize;
  Pda::PdaDmaBuffer mPdaBuffer;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_DMABUFFERPROVIDER_PDADMABUFFERPROVIDER_H_
