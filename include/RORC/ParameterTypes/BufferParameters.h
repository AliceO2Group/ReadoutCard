/// \file BufferParameters.h
/// \brief Definition of the BufferParameters
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_BUFFERPARAMETERS_H_
#define ALICEO2_INCLUDE_RORC_BUFFERPARAMETERS_H_

#include <string>

namespace AliceO2 {
namespace Rorc {
namespace BufferParameters {

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

} // namespace BufferParameters
} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_BUFFERPARAMETERS_H_
