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
      initialize(mMappedFilePages->getAddress(), parameters.size);
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
