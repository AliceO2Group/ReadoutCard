/// \file PdaBar.h
/// \brief Definition of the PdaBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ReadoutCard/BarInterface.h"
#include <pda.h>
#include "PdaDevice.h"
#include "ExceptionInternal.h"
#ifndef NDEBUG
# include <boost/type_index.hpp>
#endif

namespace AliceO2 {
namespace roc {
namespace Pda {

/// A simple wrapper around the PDA BAR object, providing some convenience functions
class PdaBar : public BarInterface
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

    virtual int getIndex() const override
    {
      return mBarNumber;
    }

    virtual size_t getSize() const override
    {
      return mBarNumber;
    }

    template<typename T>
    void barWrite(uintptr_t byteOffset, const T &value) const
    {
#ifndef NDEBUG
      std::printf("PdaBar::barWrite<%s>(address=0x%lx, value=0x%x)\n",
                  boost::typeindex::type_id<T>().pretty_name().c_str(), byteOffset, value);
      std::fflush(stdout);
#endif
      assertRange<T>(byteOffset);
      memcpy(getOffsetAddress(byteOffset), &value, sizeof(T));
    }

    template<typename T>
    T barRead(uintptr_t byteOffset) const
    {
#ifndef NDEBUG
      std::printf("PdaBar::barRead<%s>(address=0x%lx)\n",
                  boost::typeindex::type_id<T>().pretty_name().c_str(), byteOffset);
      std::fflush(stdout);
#endif
      assertRange<T>(byteOffset);
      T value;
      memcpy(&value, getOffsetAddress(byteOffset), sizeof(T));
#ifndef NDEBUG
      std::printf("PdaBar::barRead<%s>(address=0x%lx) -> 0x%x\n",
                  boost::typeindex::type_id<T>().pretty_name().c_str(), byteOffset, value);
      std::fflush(stdout);
#endif

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
