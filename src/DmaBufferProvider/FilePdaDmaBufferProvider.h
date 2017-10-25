/// \file FilePdaDmaBufferProvider.h
/// \brief Definition of the FilePdaDmaBufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "DmaBufferProvider/DmaBufferProviderInterface.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include "ReadoutCard/MemoryMappedFile.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaDmaBuffer.h"

namespace AliceO2 {
namespace roc {

/// Base class for objects that provides addresses and offsets for buffers (typically DMA buffers)
class FilePdaDmaBufferProvider : public DmaBufferProviderInterface
{
  public:
    FilePdaDmaBufferProvider(Pda::PdaDevice::PdaPciDevice pciDevice, std::string path, size_t size, int dmaBufferId,
        bool requireHugepage)
        : mMappedFile(path, size), mAddress(mMappedFile.getAddress()), mSize(mMappedFile.getSize()),
          mPdaBuffer(pciDevice, mAddress, mSize, dmaBufferId, requireHugepage)
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
} // namespace AliceO2
