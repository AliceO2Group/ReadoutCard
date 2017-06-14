/// \file MemoryMappedFile.cxx
/// \brief Implementation of the MemoryMappedFile class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/MemoryMappedFile.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace roc {

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

struct MemoryMappedFileInternal
{
    std::string fileName;
    boost::interprocess::file_mapping fileMapping;
    boost::interprocess::mapped_region mappedRegion;
    bool deleteFileOnDestruction;
};

MemoryMappedFile::MemoryMappedFile()
{
  mInternal = std::make_unique<MemoryMappedFileInternal>();
}

MemoryMappedFile::MemoryMappedFile(const std::string& fileName, size_t fileSize, bool deleteFileOnDestruction)
    : MemoryMappedFile()
{
  mInternal->fileName = fileName;
  mInternal->deleteFileOnDestruction = deleteFileOnDestruction;
  map(fileName, fileSize);
}

MemoryMappedFile::~MemoryMappedFile()
{
  if (mInternal->deleteFileOnDestruction) {
    bfs::remove(mInternal->fileName);
  }
}

void* MemoryMappedFile::getAddress() const
{
  return mInternal->mappedRegion.get_address();
}

size_t MemoryMappedFile::getSize() const
{
  return mInternal->mappedRegion.get_size();
}

void MemoryMappedFile::map(const std::string& fileName, size_t fileSize)
{
  try {
    // Check the directory exists
    {
      auto dir = bfs::path(fileName.c_str()).parent_path();
      if (!(bfs::is_directory(dir) && bfs::exists(dir))) {
        BOOST_THROW_EXCEPTION(MemoryMapException()
            << ErrorInfo::Message("Failed to open memory map file, parent directory does not exist"));
      }
    }

    // Similar operation to calling "touch" command, making sure the file exists
    try {
      std::ofstream ofs(fileName.c_str(), std::ios::app);
    }
    catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
          << ErrorInfo::Message(std::string("Failed to open memory map file: ") + e.what()));
    }

    // Resize and map file to memory
    try {
      bfs::resize_file(fileName.c_str(), fileSize);
    }
    catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
          << ErrorInfo::Message(std::string("Failed to resize memory map file: ") + e.what())
          << ErrorInfo::PossibleCauses({
            "Size not a multiple of page size",
            "Not enough memory available",
            "Not enough memory available (check 'hugeadm --pool-list')",
            "Insufficient permissions"}));
    }

    try {
      mInternal->fileMapping = bip::file_mapping(fileName.c_str(), bip::read_write);
      mInternal->mappedRegion = bip::mapped_region(mInternal->fileMapping, bip::read_write, 0, fileSize);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
          << ErrorInfo::Message(std::string("Failed to memory map file: ") + e.what())
          << ErrorInfo::PossibleCauses({
            "Not enough memory available",
            "Not enough hugepages allocated (check 'hugeadm --pool-list')"}));
    }
  }
  catch (MemoryMapException& e) {
    e << ErrorInfo::FileName(fileName) << ErrorInfo::FileSize(fileSize);
    throw;
  }
}

} // namespace roc
} // namespace AliceO2
