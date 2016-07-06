///
/// \file Page.h
/// \author Pascal Boeschoten
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
    inline Page(volatile void* address = nullptr, size_t size = 0)
        : address(address), size(size)
    {
    }

    inline volatile void* getAddress() const
    {
      return address;
    }

    inline volatile uint32_t* getAddressU32() const
    {
      return reinterpret_cast<volatile uint32_t*>(address);
    }

    inline size_t getSize() const
    {
      return size;
    }

  private:
    volatile void* address;
    size_t size;
};

} // namespace Rorc
} // namespace AliceO2
