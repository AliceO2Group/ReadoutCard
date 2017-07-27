/// \file MemoryMaps.h
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace AliceO2 {
namespace roc {
namespace Utilities {

struct MemoryMap
{
    uintptr_t addressStart; ///< Starting address of the mapping
    uintptr_t addressEnd; ///< End address of the mapping
    size_t pageSizeKiB; ///< Size of the pages. 0 if unknown.
    std::string path; ///< Pathname of the mapping.
};

/// TODO Work in progress
std::vector<MemoryMap> getMemoryMaps();

} // namespace Util
} // namespace roc
} // namespace AliceO2
