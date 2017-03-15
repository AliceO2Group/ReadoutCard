/// \file SuperpageStatus.h
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
    /// Returns true if the superpage is filled
    bool isFilled() const
    {
      return received == getSize();
    }

    /// The offset of the superpage. It's also its ID.
    size_t getOffset() const
    {
      return offset;
    }

    /// The size of the superpage
    size_t getSize() const
    {
      return size;
    }

    void* getUserData() const
    {
      return userData;
    }

    /// The amount of bytes received in the superpage
    size_t getReceived() const
    {
      return received;
    }

    /// Offset from the start of the DMA buffer to the start of the superpage. Must be ???-byte aligned.
    /// The offset will also be used as an identifier for the superpage.
    size_t offset = 0;

    /// Size of the superpage
    size_t size = 0;

    /// Pointer that users can use for whatever, e.g. to associate data with the superpage
    void* userData = nullptr;

    /// Size of the received data (will be filled in by the driver)
    size_t received = 0;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_SUPERPAGE_H_
