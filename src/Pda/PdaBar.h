/// \file PdaBar.h
/// \brief Definition of the PdaBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ReadoutCard/RegisterReadWriteInterface.h"
#include <pda.h>
#include "PdaDevice.h"
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace roc {
namespace Pda {

/// A simple wrapper around the PDA BAR object, providing some convenience functions
class PdaBar : public RegisterReadWriteInterface
{
  public:
    PdaBar();

    PdaBar(PdaDevice::PdaPciDevice pciDevice, int barNumber);

    virtual uint32_t readRegister(int index)
    {
      return barRead<uint32_t>(index * sizeof(uint32_t));
    }

    virtual void writeRegister(int index, uint32_t value)
    {
      barWrite<uint32_t>(index * sizeof(uint32_t), value);
    }

    template<typename T>
    void barWrite(uintptr_t byteOffset, const T &value) const
    {
      assertRange<T>(byteOffset);
      memcpy(getOffsetAddress(byteOffset), &value, sizeof(T));
    }

    template<typename T>
    T barRead(uintptr_t byteOffset) const
    {
      assertRange<T>(byteOffset);
      T value;
      memcpy(&value, getOffsetAddress(byteOffset), sizeof(T));
      return value;
    }

    int getBarNumber() const
    {
      return mBarNumber;
    }

    size_t getBarLength() const
    {
      return mBarLength;
    }

  private:
    template<typename T>
    bool isInRange(size_t offset) const
    {
      return (mUserspaceAddress + offset + sizeof(T)) < (mUserspaceAddress + mBarLength);
    }

    template <typename T>
    void assertRange(uintptr_t offset) const
    {
      if (!isInRange<T>(offset)) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("BAR offset out of range")
            << ErrorInfo::BarIndex(offset)
            << ErrorInfo::BarSize(getBarLength()));
      }
    }

    void* getOffsetAddress(uintptr_t byteOffset) const
    {
      return reinterpret_cast<void*>(mUserspaceAddress + byteOffset);
    }

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
} // namespace roc
} // namespace AliceO2
