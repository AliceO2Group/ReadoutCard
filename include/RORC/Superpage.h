/// \file Superpage.h
/// \brief Definition of the SuperpageStatus struct
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_SUPERPAGE_H_
#define ALICEO2_INCLUDE_RORC_SUPERPAGE_H_

#include <cstddef>

namespace AliceO2 {
namespace Rorc {

/// Simple struct for holding basic info about a superpage
struct Superpage
{
    Superpage() = default;

    Superpage(size_t offset, size_t size, void* userData = nullptr)
        : offset(offset), size(size), userData(userData)
    {
    }

    /// TODO
    /// Returns true if the superpage is ready, meaning the transfer is complete. This does not necessarily mean the
    /// superpage is filled.
    bool isReady() const
    {
      return ready;
    }

    /// Returns true if the superpage is completely filled
    bool isFilled() const
    {
      return received == getSize();
    }

    /// Offset from the start of the DMA buffer to the start of the superpage. Must be ???-byte aligned.
    size_t getOffset() const
    {
      return offset;
    }

    /// Size of the superpage in bytes
    size_t getSize() const
    {
      return size;
    }

    /// Size of the received data in bytes
    size_t getReceived() const
    {
      return received;
    }

    /// Get the user data pointer
    void* getUserData() const
    {
      return userData;
    }

    size_t offset = 0; ///< Offset from the start of the DMA buffer to the start of the superpage
    size_t size = 0; ///< Size of the superpage in bytes
    void* userData = nullptr; ///< Pointer that users can use for whatever, e.g. to associate data with the superpage
    size_t received = 0; ///< Size of the received data in bytes
    bool ready = false; ///< Indicates this superpage is ready
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_SUPERPAGE_H_
