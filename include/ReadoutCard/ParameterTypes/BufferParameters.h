// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file BufferParameters.h
/// \brief Definition of the BufferParameters
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_BUFFERPARAMETERS_H_
#define ALICEO2_INCLUDE_READOUTCARD_BUFFERPARAMETERS_H_

#include <string>

namespace AliceO2 {
namespace roc {
namespace buffer_parameters {

/// Buffer parameters for user-provided DMA buffer passed by pointer
struct Memory
{
    void* address; ///< Pointer to start of buffer
    size_t size; ///< Size in bytes of buffer
};

/// Buffer parameters for user-provided DMA buffer passed by file
struct File
{
    std::string path; ///< Path to shared memory file to be memory-mapped
    size_t size; ///< Size of shared memory file
};

/// Buffer parameters to instantiate DmaChannel without data transfer, e.g. for testing purposes.
struct Null
{
};

} // namespace buffer_parameters
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_BUFFERPARAMETERS_H_
