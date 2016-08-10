///
/// \file MemoryMappedFile.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
#pragma once

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace AliceO2 {
namespace Rorc {

/// Handles the creation and cleanup of a memory mapping of a file
class MemoryMappedFile
{
  public:
    MemoryMappedFile();
    MemoryMappedFile(const char* fileName, size_t fileSize);

    void* getAddress() const
    {
      return mMappedRegion.get_address();
    }

    size_t getSize() const
    {
      return mMappedRegion.get_size();
    }

  private:
    boost::interprocess::file_mapping mFileMapping;
    boost::interprocess::mapped_region mMappedRegion;

    void map(const char* fileName, size_t fileSize);
};

} // namespace Rorc
} // namespace AliceO2
