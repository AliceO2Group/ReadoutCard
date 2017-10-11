/// \file Hugetlbfs.cxx
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Hugetlbfs.h"
#include <fstream>
#include <sstream>
#include <boost/format.hpp>
#include "ExceptionInternal.h"
#include "Common/System.h"
#include "Utilities/Util.h"
#include "Utilities/SmartPointer.h"

namespace AliceO2 {
namespace roc {
namespace Utilities {
namespace b = boost;
namespace {
constexpr size_t SIZE_2MiB = 2*1024*1024;
constexpr size_t SIZE_1GiB = 1*1024*1024*1024;
} // Anonymous namespace

std::string getDirectory(HugepageType hugepageType)
{
  return (b::format("/var/lib/hugetlbfs/global/pagesize-%s/")
      % (hugepageType == HugepageType::Size2MiB ? "2MB" : "1GB")).str();
}

std::unique_ptr<MemoryMappedFile> tryMapFile(size_t bufferSize, std::string bufferName, bool deleteOnDestruction,
    HugepageType* allocatedHugepageType)
{
  std::unique_ptr<MemoryMappedFile> memoryMappedFile;
  HugepageType attemptHugepageType;

  // To use hugepages, the buffer size must be a multiple of 2 MiB (or 1 GiB, but we cover that with 2 MiB anyway)
  if (!Utilities::isMultiple(bufferSize, SIZE_2MiB)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Buffer size not a multiple of 2 MiB"));
  }

  // Check which hugepage size we should use
  if (Utilities::isMultiple(bufferSize, SIZE_1GiB)) {
    attemptHugepageType = HugepageType::Size1GiB;
  } else {
    attemptHugepageType = HugepageType::Size2MiB;
  }

  auto createBuffer = [&](HugepageType hugepageType) {
    // Create buffer file
    std::string bufferFilePath = b::str(
        b::format("/var/lib/hugetlbfs/global/pagesize-%1%/%2%")
        % (hugepageType == HugepageType::Size2MiB ? "2MB" : "1GB")
        % bufferName);
    Utilities::resetSmartPtr(memoryMappedFile, bufferFilePath, bufferSize, deleteOnDestruction);
    if (allocatedHugepageType) {
      *allocatedHugepageType = hugepageType;
    }
  };

  if (attemptHugepageType == HugepageType::Size1GiB) {
    try {
      createBuffer(HugepageType::Size1GiB);
    }
    catch (const MemoryMapException&) {
      // Failed to allocate buffer with 1GiB hugepages, falling back to 2MiB hugepages...
    }
  }
  if (!memoryMappedFile) {
    createBuffer(HugepageType::Size2MiB);
  }
  return memoryMappedFile;
}

}
 // namespace Util
}// namespace roc
} // namespace AliceO2
