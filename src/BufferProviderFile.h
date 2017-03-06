/// \file BufferProvider.h
/// \brief Definition of the BufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include "BufferProvider.h"
#include "RORC/MemoryMappedFile.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace Rorc {

/// Buffer provider for a buffer that must be memory-mapped from a file
class BufferProviderFile : public BufferProvider
{
  public:
    BufferProviderFile(const BufferParameters::File& parameters)
    {
      Utilities::resetSmartPtr(mMappedFilePages, parameters.path, parameters.size);
      bufferStartAddress = mMappedFilePages->getAddress();
      bufferSize = parameters.size;
      reservedOffset = parameters.reservedStart;
      reservedStartAddress = Utilities::offsetBytes(bufferStartAddress, parameters.reservedStart);
      reservedSize = parameters.reservedSize;
      dmaOffset = parameters.dmaStart;
      dmaStartAddress = Utilities::offsetBytes(bufferStartAddress, parameters.dmaStart);
      dmaSize = parameters.dmaSize;
    }

    virtual ~BufferProviderFile()
    {
    }

  private:
    /// Memory mapped file containing pages used for DMA transfer destination
    std::unique_ptr<MemoryMappedFile> mMappedFilePages;
};

} // namespace Rorc
} // namespace AliceO2
