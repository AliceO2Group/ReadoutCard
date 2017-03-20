/// \file PdaBar.h
/// \brief Definition of the PdaBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <pda.h>
#include "PdaDevice.h"
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace Rorc {
namespace Pda {

/// A simple wrapper around the PDA BAR object, providing some convenience functions
class PdaBar
{
  public:
    PdaBar();
    PdaBar(PdaDevice::PdaPciDevice pciDevice, int barNumber);

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
    bool isInRange(size_t offset) const
    {
      auto address = reinterpret_cast<volatile char*>(mUserspaceAddress);
      return &address[offset + sizeof(T)] < &address[mBarLength];
    }

    /// Get the value of a register from the BAR
    /// \param offset The byte offset of the register
    template <typename T>
    T getRegister(size_t offset) const
    {
      return at<T>(offset);
    }

    /// Set the value of a register from the BAR
    /// \param offset The byte offset of the register
    /// \param value The value to set
    template <typename T>
    void setRegister(size_t offset, T value) const
    {
      at<T>(offset) = value;
    }

    /// Get a reference to a register from the BAR
    /// \param offset The byte offset of the register
    template <typename T>
    volatile T& at(size_t offset) const
    {
      if (!isInRange<T>(offset)) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("BAR offset out of range")
            << ErrorInfo::BarIndex(offset)
            << ErrorInfo::BarSize(getBarLength()));
      }

      return *reinterpret_cast<volatile T*>(getUserspaceAddress() + offset);
    }

    void* getOffsetAddress(uintptr_t byteOffset) const
    {
      return reinterpret_cast<void*>(getUserspaceAddress() + byteOffset);
    }

    template<typename T>
    void barWrite(uintptr_t byteOffset, const T& value) const
    {
      memcpy(getOffsetAddress(byteOffset), &value, sizeof(T));
    }

    template<typename T>
    T barRead(uintptr_t byteOffset) const
    {
      T value;
      memcpy(&value, getOffsetAddress(byteOffset), sizeof(T));
      return value;
    }

    uintptr_t getUserspaceAddress() const
    {
      return mUserspaceAddress;
    }

    volatile uint32_t* getUserspaceAddressU32() const
    {
      return reinterpret_cast<volatile uint32_t*>(mUserspaceAddress);
    }

  private:
    /// PDA object for the PCI BAR
    Bar* mPdaBar;

    /// Length of the BAR
    size_t mBarLength;

    /// Index of the BAR
    int mBarNumber;

    /// Userspace addresses of the mapped BARs
    uintptr_t mUserspaceAddress;
};

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
