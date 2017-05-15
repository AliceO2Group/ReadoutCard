/// \file MemoryMaps.h
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

namespace AliceO2 {
namespace roc {
namespace Utilities {

struct MemoryMap
{
    uintptr_t startAddress;
    uintptr_t endAddress;
    size_t size;
    bool isHuge; ///< Is hugepage memory
    size_t kernelPageSize; ///< Size of the
};

/// TODO Work in progress
std::vector<MemoryMap> getMemoryMaps();

} // namespace Util
} // namespace roc
} // namespace AliceO2
