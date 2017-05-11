/// \file BarAccessor.h
/// \brief Definition of the BarAccessor class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstddef>
#include <boost/optional/optional.hpp>
#include "Cru/Constants.h"
#include "Pda/PdaBar.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace Rorc {
namespace Cru {

/// A simple wrapper object for accessing the CRU BAR
class BarAccessor
{
  public:
    BarAccessor(Pda::PdaBar* pdaBar)
        : mPdaBar(pdaBar)
    {
    }

    enum class BufferStatus
    {
        AVAILABLE,
        BUSY
    };

    /// Push a superpage into the FIFO
    /// \param pages Amount of 8 kiB pages in superpage
    /// \param busAddress Superpage PCI bus address
    void pushSuperpageDescriptor(uint32_t pages, uintptr_t busAddress)
    {
      // Set superpage address
      mPdaBar->writeRegister(Registers::SUPERPAGE_ADDRESS_HIGH, Utilities::getUpper32Bits(busAddress));
      mPdaBar->writeRegister(Registers::SUPERPAGE_ADDRESS_LOW, Utilities::getLower32Bits(busAddress));
      // Set superpage size
      mPdaBar->writeRegister(Registers::SUPERPAGE_PAGES_AVAILABLE, pages);
    }

//    uint32_t getSuperpagePushedPages(uint32_t index)
//    {
//      assert(index < SUPERPAGE_DESCRIPTORS);
//      return mPdaBar->readRegister(Registers::SUPERPAGE_PUSHED_PAGES + index);
//    }

//    BufferStatus getSuperpageBufferStatus(uint32_t index)
//    {
//      uint32_t status = mPdaBar->readRegister(Registers::SUPERPAGE_STATUS);
//      uint32_t bit = status & (0b1 << index);
//      if (bit == 0) {
//        return BufferStatus::BUSY;
//      } else {
//        return BufferStatus::AVAILABLE;
//      }
//    }

    uint32_t getSuperpageCount()
    {
      return mPdaBar->readRegister(Registers::SUPERPAGES_PUSHED);
    }

    void setDataEmulatorEnabled(bool enabled) const
    {
      mPdaBar->writeRegister(Registers::DMA_CONTROL, enabled ? 0x1 : 0x0);

      uint32_t bits = mPdaBar->readRegister(Registers::DATA_GENERATOR_CONTROL);
      if (enabled) {
        bits |= uint32_t(1);
      } else {
        bits ^= ~uint32_t(1);
      }
      mPdaBar->writeRegister(Registers::DATA_GENERATOR_CONTROL, bits);
    }

    void resetDataGeneratorCounter() const
    {
      mPdaBar->writeRegister(Registers::RESET_CONTROL, 0x2);
    }

    void resetCard() const
    {
      mPdaBar->writeRegister(Registers::RESET_CONTROL, 0x1);
    }

    void setDataGeneratorPattern(GeneratorPattern::type pattern)
    {
      uint32_t bits = mPdaBar->readRegister(Registers::DATA_GENERATOR_CONTROL);

      switch (pattern) {
        case GeneratorPattern::Incremental:
          setBit(bits, 1, true);
          setBit(bits, 2, false);
          break;
        case GeneratorPattern::Alternating:
          setBit(bits, 1, false);
          setBit(bits, 2, true);
          break;
        case GeneratorPattern::Constant:
          setBit(bits, 1, true);
          setBit(bits, 2, true);
          break;
        default:
          BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Unsupported generator pattern for CRU")
                  << ErrorInfo::GeneratorPattern(pattern));
      }

      mPdaBar->writeRegister(Registers::DATA_GENERATOR_CONTROL, bits);
    }

    uint32_t getSerialNumber() const
    {
      if (mPdaBar->getBarNumber() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Can only get serial number from BAR 2")
            << ErrorInfo::BarIndex(mPdaBar->getBarNumber()));
      }

      return mPdaBar->readRegister(Registers::SERIAL_NUMBER);
    }

    /// Get raw data from the temperature register
    uint32_t getTemperatureRaw() const
    {
      if (mPdaBar->getBarNumber() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Can only get temperature from BAR 2")
            << ErrorInfo::BarIndex(mPdaBar->getBarNumber()));
      }

      // Only use lower 10 bits
      return mPdaBar->readRegister(Registers::TEMPERATURE) & 0x3ff;
    }

    /// Converts a value from the CRU's temperature register and converts it to a °C double value.
    /// \param registerValue Value of the temperature register to convert
    /// \return Temperature value in °C or nothing if the registerValue was invalid
    boost::optional<float> convertTemperatureRaw(uint32_t registerValue) const
    {
      /// It's a 10 bit register, so: 2^10 - 1
      constexpr uint32_t REGISTER_MAX_VALUE = 1023;

      // Conversion formula from: https://documentation.altera.com/#/00045071-AA$AA00044865
      if (registerValue == 0 || registerValue > REGISTER_MAX_VALUE) {
        return boost::none;
      } else {
        float A = 693.0;
        float B = 265.0;
        float C = float(registerValue);
        return ((A * C) / 1024.0f) - B;
      }
    }

    /// Gets the temperature in °C, or nothing if the temperature value was invalid.
    /// \return Temperature value in °C or nothing if the temperature value was invalid
    boost::optional<float> getTemperatureCelsius() const
    {
      return convertTemperatureRaw(getTemperatureRaw());
    }

    uint32_t getFirmwareCompileInfo()
    {
      return mPdaBar->readRegister(Registers::FIRMWARE_COMPILE_INFO);
    }

    uint32_t getFirmwareGitHash()
    {
      if (mPdaBar->getBarNumber() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Can only get git hash from BAR 2")
            << ErrorInfo::BarIndex(mPdaBar->getBarNumber()));
      }
      return mPdaBar->readRegister(Registers::FIRMWARE_GIT_HASH);
    }

    uint32_t getFirmwareDateEpoch()
    {
      if (mPdaBar->getBarNumber() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Can only get firmware epoch from BAR 2")
            << ErrorInfo::BarIndex(mPdaBar->getBarNumber()));
      }
      return mPdaBar->readRegister(Registers::FIRMWARE_EPOCH);
    }

    uint32_t getFirmwareDate()
    {
      if (mPdaBar->getBarNumber() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Can only get firmware date from BAR 2")
            << ErrorInfo::BarIndex(mPdaBar->getBarNumber()));
      }
      return mPdaBar->readRegister(Registers::FIRMWARE_DATE);
    }

    uint32_t getFirmwareTime()
    {
      if (mPdaBar->getBarNumber() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Can only get firmware time from BAR 2")
            << ErrorInfo::BarIndex(mPdaBar->getBarNumber()));
      }
      return mPdaBar->readRegister(Registers::FIRMWARE_TIME);
    }

    uint8_t getDebugReadWriteRegister()
    {
      return mPdaBar->barRead<uint8_t>(toByteAddress(Registers::DEBUG_READ_WRITE));
    }

    void setDebugReadWriteRegister(uint8_t value)
    {
      return mPdaBar->barWrite<uint8_t>(toByteAddress(Registers::DEBUG_READ_WRITE), value);
    }

  private:
    /// Convert an index to a byte address
    size_t toByteAddress(size_t address32) const
    {
      return address32 * 4;
    }

    void setBit(uint32_t& bits, int index, bool value) const
    {
      if (value) {
        bits |= uint32_t(1) << index;
      } else {
        bits &= ~(uint32_t(1) << index);
      }
    }

    Pda::PdaBar* mPdaBar;
//    uintptr_t bar;
};

} // namespace Cru
} // namespace Rorc
} // namespace AliceO2
