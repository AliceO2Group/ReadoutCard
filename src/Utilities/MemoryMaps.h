// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file MemoryMaps.h
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_UTILITIES_MEMORYMAPS_H_
#define ALICEO2_SRC_READOUTCARD_UTILITIES_MEMORYMAPS_H_

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

#endif // ALICEO2_SRC_READOUTCARD_UTILITIES_MEMORYMAPS_H_
