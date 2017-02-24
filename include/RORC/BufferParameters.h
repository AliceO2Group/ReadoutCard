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
    void* bufferStart; ///< Pointer to start of buffer
    size_t bufferSize; ///< Size in bytes of buffer
    void* dmaStart; ///< Pointer to start of DMA region
    size_t dmaSize; ///< Size in bytes of DMA region
    void* reservedStart; ///< Pointer to start of region available for device-specific uses
    size_t reservedSize; ///< Size in bytes of region available for device-specific uses
};

/// Buffer parameters for user-provided DMA buffer passed by file
struct File
{
    std::string path; ///< Path to shared memory file to be memory-mapped
    size_t size; ///< Size of shared memory file
    size_t dmaStart; ///< Offset in bytes from start of file to start of DMA region
    size_t dmaSize; ///< Size in bytes of DMA region
    size_t reservedStart; ///< Offset in bytes from start of file to start of region available for device-specific uses
    size_t reservedSize; ///< Size in bytes of region available for device-specific uses
};

} // namespace BufferParameters
} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_BUFFERPARAMETERS_H_
