
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
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
#include "Utilities/SmartPointer.h"

namespace o2
{
namespace roc
{

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

struct MemoryMappedFileInternal {
  std::string fileName;
  boost::interprocess::file_mapping fileMapping;
  boost::interprocess::mapped_region mappedRegion;
  bool deleteFileOnDestruction;
};

MemoryMappedFile::MemoryMappedFile()
{
  mInternal = std::make_unique<MemoryMappedFileInternal>();
}

MemoryMappedFile::MemoryMappedFile(const std::string& fileName, size_t fileSize, bool deleteFileOnDestruction,
                                   bool lockMap)
  : MemoryMappedFile()
{
  mInternal->fileName = fileName;
  mInternal->deleteFileOnDestruction = deleteFileOnDestruction;

  if (lockMap) {
    // Try to acquire the lock on the file
    try {
      Utilities::resetSmartPtr(mInterprocessLock, "Alice_O2_RoC_MMF_" + fileName + "_lock");
    } catch (const boost::exception& e) {
      BOOST_THROW_EXCEPTION(LockException()
                            << ErrorInfo::Message("Couldn't lock Memory Mapped File; " + boost::diagnostic_information(e)));
    }

    try {
      mMapAcquired = map(fileName, fileSize);
    } catch (const boost::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
                            << ErrorInfo::Message(boost::diagnostic_information(e)));
    }
  }
}

MemoryMappedFile::~MemoryMappedFile()
{
  if (mInternal->deleteFileOnDestruction && mMapAcquired) {
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

std::string MemoryMappedFile::getFileName() const
{
  return mInternal->fileName;
}

bool MemoryMappedFile::map(const std::string& fileName, size_t fileSize)
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

    // We don't care if the file exists.
    // Locks are in place that make sure we don't get here unless we're allowed to
    // Check the file exists
    /*{
      if (bfs::exists(fileName) && (bfs::file_size(fileName) != fileSize)) {
        BOOST_THROW_EXCEPTION(MemoryMapException()
            << ErrorInfo::Message("File already exists, but mismatching file size. Resizing dangerous."));
      }
    }*/

    // Similar operation to calling "touch" command, making sure the file exists
    try {
      std::ofstream ofs(fileName.c_str(), std::ios::app);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
                            << ErrorInfo::Message(std::string("Failed to open memory map file: ") + e.what()));
    }

    // Resize and map file to memory
    try {
      bfs::resize_file(fileName.c_str(), fileSize);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
                            << ErrorInfo::Message(std::string("Failed to resize memory map file: ") + e.what())
                            << ErrorInfo::PossibleCauses({ "Size not a multiple of page size",
                                                           "Not enough memory available",
                                                           "Not enough hugepages allocated (check 'hugeadm --pool-list')",
                                                           "Insufficient permissions",
                                                           "Stale hugepage / uio_pci_dma resources (run 'roc-cleanup')" }));
    }

    try {
      mInternal->fileMapping = bip::file_mapping(fileName.c_str(), bip::read_write);
      mInternal->mappedRegion = bip::mapped_region(mInternal->fileMapping, bip::read_write, 0, fileSize);
    } catch (const std::exception& e) {
      BOOST_THROW_EXCEPTION(MemoryMapException()
                            << ErrorInfo::Message(std::string("Failed to memory map file: ") + e.what())
                            << ErrorInfo::PossibleCauses({ "Not enough memory available",
                                                           "Not enough hugepages allocated (check 'hugeadm --pool-list')",
                                                           "Stale hugepage / uio_pci_dma resources (run 'roc-cleanup')" }));
    }
  } catch (MemoryMapException& e) {
    e << ErrorInfo::FileName(fileName) << ErrorInfo::FileSize(fileSize);
    throw;
  }

  return true;
}

} // namespace roc
} // namespace o2
