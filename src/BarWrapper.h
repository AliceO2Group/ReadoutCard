///
/// \file BarWrapper.h
/// \author Pascal Boeschoten
///

#pragma once

#include <pda.h>

#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

/// A simple wrapper around the PDA BAR object, providing some convenience functions
class BarWrapper
{
  public:
    BarWrapper();
    BarWrapper(PciDevice* pciDevice, int channel);

    inline size_t getBarLength() const
    {
      return barLength;
    }

    inline Bar* getPdaBar() const
    {
      return pdaBar;
    }

    inline volatile void* getUserspaceAddress() const
    {
      return userspaceAddress;
    }

    inline volatile uint32_t* getUserspaceAddressU32() const
    {
      return reinterpret_cast<volatile uint32_t*>(userspaceAddress);
    }

    /// Get reference to register
    inline volatile uint32_t& operator[](size_t i)
    {
#ifndef NDEBUG
      if (i >= barLength) {
        ALICEO2_RORC_THROW_EXCEPTION("BAR index out of bounds");
      }
#endif

      return getUserspaceAddressU32()[i];
    }

  private:
    /// PDA object for the PCI BAR
    Bar* pdaBar;

    /// Length of the BAR
    size_t barLength;

    /// Userspace addresses of the mapped BARs
    volatile void* userspaceAddress;
};

} // namespace Rorc
} // namespace AliceO2
