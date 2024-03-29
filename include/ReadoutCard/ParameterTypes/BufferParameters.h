
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
/// \file BufferParameters.h
/// \brief Definition of the BufferParameters
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_BUFFERPARAMETERS_H_
#define O2_READOUTCARD_INCLUDE_BUFFERPARAMETERS_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>

namespace o2
{
namespace roc
{
namespace buffer_parameters
{

/// Buffer parameters for user-provided DMA buffer passed by pointer
struct Memory {
  void* address; ///< Pointer to start of buffer
  size_t size;   ///< Size in bytes of buffer
};

/// Buffer parameters for user-provided DMA buffer passed by file
struct File {
  std::string path; ///< Path to shared memory file to be memory-mapped
  size_t size;      ///< Size of shared memory file
};

/// Buffer parameters to instantiate DmaChannel without data transfer, e.g. for testing purposes.
struct Null {
};

} // namespace buffer_parameters
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_BUFFERPARAMETERS_H_
