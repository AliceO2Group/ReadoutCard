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
      bufferStartAddress = reinterpret_cast<uintptr_t>(mMappedFilePages->getAddress());
      bufferSize = parameters.size;

      reservedOffset = parameters.reservedStart;
      reservedSize = parameters.reservedSize;
      reservedStartAddress = bufferStartAddress + reservedOffset;
      if ((reservedStartAddress + reservedSize) > (bufferStartAddress + bufferSize)) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Reserved region out of range"));
      }

      dmaOffset = parameters.dmaStart;
      dmaSize = parameters.dmaSize;
      dmaStartAddress = bufferStartAddress + dmaOffset;
      if ((dmaStartAddress + reservedSize) > (bufferStartAddress + bufferSize)) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("DMA region out of range"));
      }
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
