/// \file BarAccessor.h
/// \brief Definition of the BarAccessor class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstddef>
#include <boost/optional/optional.hpp>
#include "Cru/Constants.h"
#include "Cru/FirmwareFeatures.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/BarInterface.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {
namespace Cru {

/// A simple wrapper object for accessing the CRU BAR
class BarAccessor
{
  public:
    BarAccessor(BarInterface* bar)
        : mBar(bar)
    {
    }

    enum class BufferStatus
    {
        AVAILABLE,
        BUSY
    };

    /// Push a superpage into the FIFO of a link
    /// \param link Link number
    /// \param pages Amount of 8 kiB pages in superpage
    /// \param busAddress Superpage PCI bus address
    void pushSuperpageDescriptor(uint32_t link, uint32_t pages, uintptr_t busAddress)
    {
      // Set superpage address. These writes are buffered on the firmware side.
      mBar->writeRegister(Registers::LINK_SUPERPAGE_ADDRESS_HIGH.get(link).index,
          Utilities::getUpper32Bits(busAddress));
      mBar->writeRegister(Registers::LINK_SUPERPAGE_ADDRESS_LOW.get(link).index,
          Utilities::getLower32Bits(busAddress));
      // Set superpage size. This write signals the push of the descriptor into the link's FIFO.
      mBar->writeRegister(Registers::LINK_SUPERPAGE_SIZE.get(link).index, pages);
    }

    /// Get amount of superpages pushed by a link
    /// \param link Link number
    uint32_t getSuperpageCount(uint32_t link)
    {
      return mBar->readRegister(Registers::LINK_SUPERPAGES_PUSHED.get(link).index);
    }

    /// Enables the data emulator
    /// \param enabled true for enabled
    void setDataEmulatorEnabled(bool enabled) const
    {
      mBar->writeRegister(Registers::DMA_CONTROL.index, enabled ? 0x1 : 0x0);
      uint32_t bits = mBar->readRegister(Registers::DATA_GENERATOR_CONTROL.index);
      setDataGeneratorEnableBits(bits, enabled);
      mBar->writeRegister(Registers::DATA_GENERATOR_CONTROL.index, bits);
    }

    /// Resets the data generator counter
    void resetDataGeneratorCounter() const
    {
      mBar->writeRegister(Registers::RESET_CONTROL.index, 0x2);
    }

    /// Performs a general reset of the card
    void resetCard() const
    {
      mBar->writeRegister(Registers::RESET_CONTROL.index, 0x1);
    }

    /// Sets the pattern for the card's internal data generator
    /// \param pattern Data generator pattern
    /// \param size Size in bytes
    /// \param randomEnabled Give true to enable random data size. In this case, the given size is the maximum size (?)
    void setDataGeneratorPattern(GeneratorPattern::type pattern, size_t size, bool randomEnabled)
    {
      uint32_t bits = mBar->readRegister(Registers::DATA_GENERATOR_CONTROL.index);
      setDataGeneratorPatternBits(bits, pattern);
      setDataGeneratorSizeBits(bits, size);
      setDataGeneratorRandomSizeBits(bits, randomEnabled);
      mBar->writeRegister(Registers::DATA_GENERATOR_CONTROL.index, bits);
    }

    /// Injects a single error into the generated data stream
    void dataGeneratorInjectError()
    {
      mBar->writeRegister(Registers::DATA_GENERATOR_CONTROL.index, Registers::DATA_GENERATOR_CONTROL_CMD_INJECT_ERROR);
    }

    /// Sets the data source for the DMA
    void setDataSource(uint32_t source)
    {
      mBar->writeRegister(Registers::DATA_SOURCE_SELECT.index, source);
    }

    /// Gets the serial number from the card.
    /// Note that not all firmwares have a serial number. You should make sure this firmware feature is enabled before
    /// calling this function, or the card may crash. See getFirmwareFeatures().
    uint32_t getSerialNumber() const
    {
      assertBarIndex(2, "Can only get serial number from BAR 2");
      auto serial = mBar->readRegister(Registers::SERIAL_NUMBER.index);
      if (serial == 0xFfffFfff) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("CRU reported invalid serial number 0xffffffff, "
            "a fatal error may have occurred"));
      }
      return serial;
    }

    /// Get raw data from the temperature register
    uint32_t getTemperatureRaw() const
    {
      assertBarIndex(2, "Can only get temperature from BAR 2");
      // Only use lower 10 bits
      return mBar->readRegister(Registers::TEMPERATURE.index) & 0x3ff;
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
      assertBarIndex(0, "Can only get firmware compile info from BAR 0");
      return mBar->readRegister(Registers::FIRMWARE_COMPILE_INFO.index);
    }

    uint32_t getFirmwareGitHash()
    {
      assertBarIndex(2, "Can only get git hash from BAR 2");
      return mBar->readRegister(Registers::FIRMWARE_GIT_HASH.index);
    }

    uint32_t getFirmwareDateEpoch()
    {
      assertBarIndex(2, "Can only get firmware epoch from BAR 2");
      return mBar->readRegister(Registers::FIRMWARE_EPOCH.index);
    }

    uint32_t getFirmwareDate()
    {
      assertBarIndex(2, "Can only get firmware date from BAR 2");
      return mBar->readRegister(Registers::FIRMWARE_DATE.index);
    }

    uint32_t getFirmwareTime()
    {
      assertBarIndex(2, "Can only get firmware time from BAR 2");
      return mBar->readRegister(Registers::FIRMWARE_TIME.index);
    }

    /// Get the enabled features for the card's firmware.
    FirmwareFeatures getFirmwareFeatures()
    {
      assertBarIndex(0, "Can only get firmware features from BAR 0");
      return convertToFirmwareFeatures(mBar->readRegister(Registers::FIRMWARE_FEATURES.index));
    }

    static FirmwareFeatures convertToFirmwareFeatures(uint32_t reg)
    {
      FirmwareFeatures features;
      uint32_t safeword = Utilities::getBits(reg, 0, 15);
      if (safeword == 0x5afe) {
        // Standalone firmware
        auto enabled = [&](int i){ return Utilities::getBit(reg, i) == 0; };
        features.standalone = true;
        features.dataSelection = enabled(16);
        features.temperature = enabled(17);
        features.serial = enabled(18);
        features.firmwareInfo = enabled(19);
      } else {
        // Integrated firmware
        features.standalone = false;
        features.temperature = true;
        features.dataSelection = true;
        features.serial = true;
        features.firmwareInfo = true;
      }
      return features;
    }

    /// Sets the bits for the data generator pattern in the given integer
    /// \param bits Integer with bits to set
    /// \param pattern Pattern to set
    static void setDataGeneratorPatternBits(uint32_t& bits, GeneratorPattern::type pattern)
    {
      switch (pattern) {
        case GeneratorPattern::Incremental:
          Utilities::setBit(bits, 1, true);
          Utilities::setBit(bits, 2, false);
          break;
        case GeneratorPattern::Alternating:
          Utilities::setBit(bits, 1, false);
          Utilities::setBit(bits, 2, true);
          break;
        case GeneratorPattern::Constant:
          Utilities::setBit(bits, 1, true);
          Utilities::setBit(bits, 2, true);
          break;
        default:
          BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Unsupported generator pattern for CRU")
                                            << ErrorInfo::GeneratorPattern(pattern));
      }
    }

    /// Sets the bits for the data generator size in the given integer
    /// \param bits Integer with bits to set
    /// \param size Size to set
    static void setDataGeneratorSizeBits(uint32_t& bits, size_t size)
    {
      if (!Utilities::isMultiple(size, 32ul)) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Unsupported generator data size for CRU; must be multiple of 32 bytes")
            << ErrorInfo::GeneratorEventLength(size));
      }

      if (size < 32 || size > Cru::DMA_PAGE_SIZE) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Unsupported generator data size for CRU; must be >= 32 bytes and <= 8 KiB")
            << ErrorInfo::GeneratorEventLength(size));
      }

      // We set the size in 256-bit (32 byte) words and do -1 because that's how it works.
      uint32_t sizeValue = ((size/32) - 1) << 8;
      bits &= ~(uint32_t(0xff00));
      bits += sizeValue;
    }

    /// Sets the bits for the data generator enabled in the given integer
    /// \param bits Integer with bits to set
    /// \param enabled Generator enabled or not
    static void setDataGeneratorEnableBits(uint32_t& bits, bool enabled)
    {
      Utilities::setBit(bits, 0, enabled);
    }

    /// Sets the bits for the data generator random size in the given integer
    /// \param bits Integer with bits to set
    /// \param enabled Random enabled or not
    static void setDataGeneratorRandomSizeBits(uint32_t& bits, bool enabled)
    {
      Utilities::setBit(bits, 16, enabled);
    }

  private:
    /// Checks if this is the correct BAR. Used to check for BAR 2 for special functions.
    void assertBarIndex(int index, std::string message) const
    {
      if (mBar->getIndex() != index) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(message) << ErrorInfo::BarIndex(mBar->getIndex()));
      }
    }

    BarInterface* mBar;
};

} // namespace Cru
} // namespace roc
} // namespace AliceO2
