/// \file SuperpageStatus.h
/// \brief Definition of the SuperpageStatus struct
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_SUPERPAGESTATUS_H_
#define ALICEO2_INCLUDE_RORC_SUPERPAGESTATUS_H_

#include <cstddef>

namespace AliceO2 {
namespace Rorc {

/// Simple struct for holding the status of a superpage
struct SuperpageStatus
{
    /// Returns true if the superpage is filled
    bool isFilled() const
    {
      return confirmedPages == maxPages;
    }

    /// Current amount of confirmed transferred pages in the superpage. Note that more may have been transferred
    /// by the time you read this.
    int getConfirmedPageCount() const
    {
      return confirmedPages;
    }

    /// Maximum amount of pages in the superpage.
    int getMaxPageCount() const
    {
      return maxPages;
    }

    /// The offset of the superpage. It's also its ID.
    size_t getOffset() const
    {
      return offset;
    }

    size_t offset;
    int confirmedPages;
    int maxPages;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_SUPERPAGESTATUS_H_
