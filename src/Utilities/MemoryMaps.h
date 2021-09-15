
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
/// \file MemoryMaps.h
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_UTILITIES_MEMORYMAPS_H_
#define O2_READOUTCARD_SRC_UTILITIES_MEMORYMAPS_H_

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace o2
{
namespace roc
{
namespace Utilities
{

struct MemoryMap {
  uintptr_t addressStart; ///< Starting address of the mapping
  uintptr_t addressEnd;   ///< End address of the mapping
  size_t pageSizeKiB;     ///< Size of the pages. 0 if unknown.
  std::string path;       ///< Pathname of the mapping.
};

/// TODO Work in progress
std::vector<MemoryMap> getMemoryMaps();

} // namespace Utilities
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_UTILITIES_MEMORYMAPS_H_
