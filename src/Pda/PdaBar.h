/// \file PdaBar.h
/// \brief Definition of the PdaBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <pda.h>

#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {
namespace Pda {

/// A simple wrapper around the PDA BAR object, providing some convenience functions
class PdaBar
{
  public:
    PdaBar();
    PdaBar(PciDevice* pciDevice, int barNumber);

    int getBarNumber() const
    {
      return mBarNumber;
    }

    size_t getBarLength() const
    {
      return mBarLength;
    }

    Bar* getPdaBar() const
    {
      return mPdaBar;
    }

    template <typename T>
    bool isInRange(size_t index) const
    {
      auto address = reinterpret_cast<volatile char*>(mUserspaceAddress);
      return address[index + sizeof(T)] < address[mBarLength];
    }

    /// Get the value of a register from the BAR
    /// \param index The byte index of the register
    template <typename T>
    T getRegister(size_t index) const
    {
      return at<T>(index);
    }

    /// Set the value of a register from the BAR
    /// \param index The byte index of the register
    /// \param
    template <typename T> const
    void setRegister(size_t index, T value)
    {
      at<T>(index) = value;
    }

    /// Get a reference to a register from the BAR
    /// \param index The byte index of the register
    template <typename T>
    volatile T& at(size_t index) const
    {
      if (!isInRange<T>(index)) {
        BOOST_THROW_EXCEPTION(Exception()
            << errinfo_rorc_error_message("BAR index out of range")
            << errinfo_rorc_bar_index(index)
            << errinfo_rorc_bar_size(getBarLength()));
      }
      return *reinterpret_cast<volatile T*>(reinterpret_cast<volatile char*>(getUserspaceAddress())[index]);
    }

    volatile void* getUserspaceAddress() const
    {
      return mUserspaceAddress;
    }

    volatile uint32_t* getUserspaceAddressU32() const
    {
      return reinterpret_cast<volatile uint32_t*>(mUserspaceAddress);
    }

    /// Get reference to register
    volatile uint32_t& operator[](size_t i)
    {
      return getUserspaceAddressU32()[i];
    }

  private:
    /// PDA object for the PCI BAR
    Bar* mPdaBar;

    /// Length of the BAR
    size_t mBarLength;

    /// Index of the BAR
    int mBarNumber;

    /// Userspace addresses of the mapped BARs
    volatile void* mUserspaceAddress;
};

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
