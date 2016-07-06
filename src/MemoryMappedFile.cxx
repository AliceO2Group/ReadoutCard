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
  // Similar operation to calling "touch" command, making sure the file exists
  try {
    std::ofstream ofs(fileName, std::ios::app);
  } catch (std::exception& e) {
    BOOST_THROW_EXCEPTION(MemoryMapException()
        << errinfo_aliceO2_rorc_generic_message("Failed to open memory map file")
        << errinfo_aliceO2_rorc_filename(std::string(fileName))
        << errinfo_aliceO2_rorc_filesize(fileSize));
  }

  // Resize and map file to memory
  try {
    bfs::resize_file(fileName, fileSize);
  } catch (std::exception& e) {
    BOOST_THROW_EXCEPTION(MemoryMapException()
        << errinfo_aliceO2_rorc_generic_message("Failed to resize memory map file")
        << errinfo_aliceO2_rorc_possible_causes(
            "Size not a multiple of page size (possibly multiple of hugepages); "
            "Not enough memory available (check hugepage allocation)")
        << errinfo_aliceO2_rorc_filename(std::string(fileName))
        << errinfo_aliceO2_rorc_filesize(fileSize));
  }

  try {
    fmap = bip::file_mapping(fileName, bip::read_write);
    region = bip::mapped_region(fmap, bip::read_write, 0, fileSize);
  } catch (std::exception& e) {
    BOOST_THROW_EXCEPTION(MemoryMapException()
        << errinfo_aliceO2_rorc_generic_message("Failed to memory map file")
        << errinfo_aliceO2_rorc_filename(std::string(fileName))
        << errinfo_aliceO2_rorc_filesize(fileSize));
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
