/// \file BufferProvider.h
/// \brief Definition of the BufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include "BufferProvider.h"
#include "ReadoutCard/MemoryMappedFile.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {

/// Buffer provider for a buffer that must be memory-mapped from a file
class BufferProviderFile : public BufferProvider
{
  public:
    BufferProviderFile(const buffer_parameters::File& parameters)
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

} // namespace roc
} // namespace AliceO2
