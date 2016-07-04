///
/// \file Page.h
/// \author Pascal Boeschoten
///

#pragma once

#include <cstdint>

namespace AliceO2 {
namespace Rorc {

class Page
{
  public:
    inline Page(volatile void* address = nullptr)
        : address(address)
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

  private:
    volatile void* address;
};

} // namespace Rorc
} // namespace AliceO2
