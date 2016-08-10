///
/// \file Page.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <cstdint>
#include <cstddef>

namespace AliceO2 {
namespace Rorc {

/// A simple data holder class. It represents a page that was transferred from the RORC.
class Page
{
  public:
    Page(volatile void* address = nullptr, size_t size = 0)
        : address(address), size(size)
    {
    }

    /// Get the userspace address of the page
    volatile void* getAddress() const
    {
      return address;
    }

    /// Get the userspace address of the page as a uint32_t pointer
    volatile uint32_t* getAddressU32() const
    {
      return reinterpret_cast<volatile uint32_t*>(address);
    }

    /// Get the size of the page in bytes
    size_t getSize() const
    {
      return size;
    }

    // TODO
//    int32_t operator[](size_t index)
//    {
//      auto max = index * sizeof(int32_t);
//      if (max >= size) {
//        BOOST_THROW_EXCEPTION(OutOfRangeException()
//            << errinfo_rorc_generic_message("Index out of page range")
//            << errinfo_rorc_index(index)
//            << errinfo_rorc_range(max));
//      } else {
//        return *(reinterpret_cast<volatile int32_t*>(address));
//      }
//    }

  private:
    volatile void* const address;
    const size_t size;
};

} // namespace Rorc
} // namespace AliceO2
