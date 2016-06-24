///
/// \file MemoryMappedFile.h
/// \author Pascal Boeschoten
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
    void map(const char* fileName, size_t fileSize);
    void* getAddress();
    size_t getSize();

  private:
    boost::interprocess::file_mapping fmap;
    boost::interprocess::mapped_region region;
};

} // namespace Rorc
} // namespace AliceO2
