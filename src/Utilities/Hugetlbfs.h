/// \file Hugetlbfs.h
/// \brief Implementation of functions for NUMA
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_UTILITIES_HUGETLBFS_H_
#define ALICEO2_SRC_READOUTCARD_UTILITIES_HUGETLBFS_H_

#include <memory>
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/MemoryMappedFile.h"

namespace AliceO2 {
namespace roc {
namespace Utilities {

enum class HugepageType
{
    Size2MiB,
    Size1GiB
};

/// Get the directory where you can create memory mapped files for the given hugepage type
/// Assumes global mounts have been created using `hugeadm --create-global-mounts`.
std::string getDirectory(HugepageType hugepageType);

/// Try to allocate and map a file in the hugetlbfs
/// Assumes global mounts have been created using `hugeadm --create-global-mounts`.
///
/// Note that the file may end up in either the 2MiB or 1GiB hugetlbfs, depending on circumstances. You can get the full
/// file path from the resulting MemoryMappedFile if you need to know it.
/// \param bufferSize The size of the buffer to allocate
/// \param bufferName The name of the file
/// \param deleteFileOnDesctruction Passed to MemoryMappedFile constructor, determines if the file is deleted on
///        destruction of the MemoryMappedFile.
/// \param allocatedHugepageType Optional argument, set to a HugepageType if you must know what type of hugepage was
///        allocated.
std::unique_ptr<MemoryMappedFile> tryMapFile(size_t bufferSize, std::string bufferName, bool deleteFileOnDestruction,
    HugepageType* allocatedHugepageType = nullptr);

} // namespace Util
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_UTILITIES_HUGETLBFS_H_