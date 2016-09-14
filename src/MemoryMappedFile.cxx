///
/// \file MemoryMappedFile.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "MemoryMappedFile.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include "RORC/Exception.h"


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
  // Check the directory exists
  {
    auto dir = bfs::path(fileName).parent_path();
    if (!(bfs::is_directory(dir) && bfs::exists(dir))) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
          << errinfo_rorc_error_message("Failed to open memory map file, parent directory does not exist")
          << errinfo_rorc_filename(std::string(fileName))
          << errinfo_rorc_filesize(fileSize));
    }
  }

  // Similar operation to calling "touch" command, making sure the file exists
  try {
    std::ofstream ofs(fileName, std::ios::app);
  } catch (const std::exception& e) {
    BOOST_THROW_EXCEPTION(MemoryMapException()
        << errinfo_rorc_error_message("Failed to open memory map file")
        << errinfo_rorc_filename(std::string(fileName))
        << errinfo_rorc_filesize(fileSize));
  }

  // Resize and map file to memory
  try {
    bfs::resize_file(fileName, fileSize);
  } catch (const std::exception& e) {
    BOOST_THROW_EXCEPTION(MemoryMapException()
        << errinfo_rorc_error_message("Failed to resize memory map file")
        << errinfo_rorc_possible_causes({
            "Size not a multiple of page size (possibly multiple of hugepages); ",
            "Not enough memory available (check hugepage allocation)"})
        << errinfo_rorc_filename(std::string(fileName))
        << errinfo_rorc_filesize(fileSize));
  }

  try {
    mFileMapping = bip::file_mapping(fileName, bip::read_write);
    mMappedRegion = bip::mapped_region(mFileMapping, bip::read_write, 0, fileSize);
  } catch (const std::exception& e) {
    BOOST_THROW_EXCEPTION(MemoryMapException()
        << errinfo_rorc_error_message("Failed to memory map file")
        << errinfo_rorc_possible_causes({"Not enough memory available (possibly not enough hugepages allocated)"})
        << errinfo_rorc_filename(std::string(fileName))
        << errinfo_rorc_filesize(fileSize));
  }
}

} // namespace Rorc
} // namespace AliceO2
