///
/// \file MemoryMappedFile.cxx
/// \author Pascal Boeschoten
///

#include "MemoryMappedFile.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include "RorcException.h"


namespace AliceO2 {
namespace Rorc {

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

MemoryMappedFile::MemoryMappedFile()
{
}

MemoryMappedFile::MemoryMappedFile(const char* fileName, size_t fileSize)
{
  map(fileName, fileSize);
}

void MemoryMappedFile::map(const char* fileName, size_t fileSize)
{
  std::ofstream ofs(fileName, std::ios::app);

  // Resize and map file to memory
  bfs::resize_file(fileName, fileSize);
  fmap = bip::file_mapping(fileName, bip::read_write);
  region = bip::mapped_region(fmap, bip::read_write, 0, fileSize);

  if (region.get_address() == nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to memory map file");
  }
};

void* MemoryMappedFile::getAddress()
{
  return region.get_address();
}

size_t MemoryMappedFile::getSize()
{
  return region.get_size();
}

} // namespace Rorc
} // namespace AliceO2
