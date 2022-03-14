
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file FilePdaDmaBufferProvider.h
/// \brief Definition of the FilePdaDmaBufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_DMABUFFERPROVIDER_FILEPDADMABUFFERPROVIDER_H_
#define O2_READOUTCARD_SRC_DMABUFFERPROVIDER_FILEPDADMABUFFERPROVIDER_H_

#include "DmaBufferProvider/DmaBufferProviderInterface.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include "ReadoutCard/MemoryMappedFile.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaDmaBuffer.h"

namespace o2
{
namespace roc
{

/// Implementation of the DmaBufferProviderInterface for file-based memory mapped DMA buffers registered with PDA
class FilePdaDmaBufferProvider : public DmaBufferProviderInterface
{
 public:
  FilePdaDmaBufferProvider(PciDevice* pciDevice, std::string path, size_t size, int dmaBufferId, SerialId serialId,
                           bool requireHugepage)
    : mMappedFile(path, size), mAddress(mMappedFile.getAddress()), mSize(mMappedFile.getSize()), mPdaBuffer(pciDevice, mAddress, mSize, dmaBufferId, serialId, requireHugepage)
  {
  }

  virtual ~FilePdaDmaBufferProvider() = default;

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
  MemoryMappedFile mMappedFile;
  void* mAddress;
  size_t mSize;
  Pda::PdaDmaBuffer mPdaBuffer;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_DMABUFFERPROVIDER_FILEPDADMABUFFERPROVIDER_H_
