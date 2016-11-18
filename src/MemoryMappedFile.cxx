/// \file MemoryMappedFile.cxx
/// \brief Implementation of the MemoryMappedFile class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

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

MemoryMappedFile::MemoryMappedFile(const std::string& fileName, size_t fileSize)
{
  map(fileName, fileSize);
}

void MemoryMappedFile::map(const std::string& fileName, size_t fileSize)
{
  try {
    // Check the directory exists
    {
      auto dir = bfs::path(fileName.c_str()).parent_path();
      if (!(bfs::is_directory(dir) && bfs::exists(dir))) {
        BOOST_THROW_EXCEPTION(MemoryMapException()
            << errinfo_rorc_error_message("Failed to open memory map file, parent directory does not exist"));
      }
    }

    // Similar operation to calling "touch" command, making sure the file exists
    try {
      std::ofstream ofs(fileName.c_str(), std::ios::app);
    }
    catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
          << errinfo_rorc_error_message(std::string("Failed to open memory map file: ") + e.what()));
    }

    // Resize and map file to memory
    try {
      bfs::resize_file(fileName.c_str(), fileSize);
    }
    catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
          << errinfo_rorc_error_message(std::string("Failed to resize memory map file: ") + e.what())
          << errinfo_rorc_possible_causes({
              "Size not a multiple of page size (possibly multiple of hugepages); ",
              "Not enough memory available (check hugepage allocation)",
              "Insufficient permissions"}));
    }

    try {
      mFileMapping = bip::file_mapping(fileName.c_str(), bip::read_write);
      mMappedRegion = bip::mapped_region(mFileMapping, bip::read_write, 0, fileSize);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
          << errinfo_rorc_error_message(std::string("Failed to memory map file: ") + e.what())
          << errinfo_rorc_possible_causes({"Not enough memory available (possibly not enough hugepages allocated)"}));
    }
  }
  catch (MemoryMapException& e) {
    e << errinfo_rorc_filename(fileName) << errinfo_rorc_filesize(fileSize);
    throw;
  }
}

} // namespace Rorc
} // namespace AliceO2
