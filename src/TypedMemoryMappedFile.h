///
/// \file TypedMemoryMappedFile.h
/// \author Pascal Boeschoten
///
#pragma once

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "MemoryMappedFile.h"

namespace AliceO2 {
namespace Rorc {

/// Handles the creation and cleanup of a memory mapping of a file
template<typename T>
class TypedMemoryMappedFile
{
  public:
    TypedMemoryMappedFile(const char* fileName)
    {
      mf.map(fileName, sizeof(T));
    }

    void* getAddress()
    {
      return mf.getAddress();
    }

    size_t getSize()
    {
      return mf.getSize();
    }

    T* get()
    {
      return reinterpret_cast<T*>(mf.getAddress());
    }

  private:
    MemoryMappedFile mf;
};

} // namespace Rorc
} // namespace AliceO2
