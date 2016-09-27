/// \file TypedMemoryMappedFile.h
/// \brief Definition of the TypedMemoryMappedFile class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "MemoryMappedFile.h"

namespace AliceO2 {
namespace Rorc {

/// Wrapper around MemoryMappedFile to provide a typed interface. Note that the user is responsible for making sure
/// the type is appropriate for type aliasing into a memory mapped region.
template<typename T>
class TypedMemoryMappedFile
{
  public:
    TypedMemoryMappedFile(const char* fileName)
      : mMemoryMappedFile(fileName, sizeof(T))
    {
    }

    void* getAddress() const
    {
      return mMemoryMappedFile.getAddress();
    }

    size_t getSize() const
    {
      return mMemoryMappedFile.getSize();
    }

    T* get() const
    {
      return reinterpret_cast<T*>(mMemoryMappedFile.getAddress());
    }

  private:
    MemoryMappedFile mMemoryMappedFile;
};

} // namespace Rorc
} // namespace AliceO2
