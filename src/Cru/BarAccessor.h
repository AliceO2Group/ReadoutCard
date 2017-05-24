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
      auto offset = Registers::LINK_SUPERPAGE_OFFSET * link;

      // Set superpage address. These writes are buffered on the firmware side.
      mBar->writeRegister(offset + Registers::LINK_BASE_SUPERPAGE_ADDRESS_HIGH,
          Utilities::getUpper32Bits(busAddress));
      mBar->writeRegister(offset + Registers::LINK_BASE_SUPERPAGE_ADDRESS_LOW,
          Utilities::getLower32Bits(busAddress));
      // Set superpage size. This write signals the push of the descriptor into the link's FIFO.
      mBar->writeRegister(offset + Registers::LINK_BASE_SUPERPAGE_SIZE, pages);
    }

    /// Get amount of superpages pushed by a link
    /// \param link Link number
    uint32_t getSuperpageCount(uint32_t link)
    {
      return mBar->readRegister(Registers::LINK_SUPERPAGE_OFFSET * link + Registers::LINK_BASE_SUPERPAGES_PUSHED);
    }

    void setDataEmulatorEnabled(bool enabled) const
    {
      mBar->writeRegister(Registers::DMA_CONTROL, enabled ? 0x1 : 0x0);
      uint32_t bits = mBar->readRegister(Registers::DATA_GENERATOR_CONTROL);
      setDataGeneratorEnableBits(bits, enabled);
      mBar->writeRegister(Registers::DATA_GENERATOR_CONTROL, bits);
    }

    void resetDataGeneratorCounter() const
    {
      mBar->writeRegister(Registers::RESET_CONTROL, 0x2);
    }

    void resetCard() const
    {
      mBar->writeRegister(Registers::RESET_CONTROL, 0x1);
    }

    void setDataGeneratorPattern(GeneratorPattern::type pattern, size_t size, bool randomEnabled)
    {
      uint32_t bits = mBar->readRegister(Registers::DATA_GENERATOR_CONTROL);
      setDataGeneratorPatternBits(bits, pattern);
      setDataGeneratorSizeBits(bits, size);
      setDataGeneratorRandomSizeBits(bits, randomEnabled);
      mBar->writeRegister(Registers::DATA_GENERATOR_CONTROL, bits);
    }

    void dataGeneratorInjectError()
    {
      mBar->writeRegister(Registers::DATA_GENERATOR_CONTROL, Registers::DATA_GENERATOR_CONTROL_CMD_INJECT_ERROR);
    }

    uint32_t getSerialNumber() const
    {
      assertBarIndex(2, "Can only get serial number from BAR 2");
      auto serial = mBar->readRegister(Registers::SERIAL_NUMBER);
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
      return mBar->readRegister(Registers::TEMPERATURE) & 0x3ff;
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
      return mBar->readRegister(Registers::FIRMWARE_COMPILE_INFO);
    }

    uint32_t getFirmwareGitHash()
    {
      assertBarIndex(2, "Can only get git hash from BAR 2");
      return mBar->readRegister(Registers::FIRMWARE_GIT_HASH);
    }

    uint32_t getFirmwareDateEpoch()
    {
      assertBarIndex(2, "Can only get firmware epoch from BAR 2");
      return mBar->readRegister(Registers::FIRMWARE_EPOCH);
    }

    uint32_t getFirmwareDate()
    {
      assertBarIndex(2, "Can only get firmware date from BAR 2");
      return mBar->readRegister(Registers::FIRMWARE_DATE);
    }

    uint32_t getFirmwareTime()
    {
      assertBarIndex(2, "Can only get firmware time from BAR 2");
      return mBar->readRegister(Registers::FIRMWARE_TIME);
    }

    FirmwareFeatures getFirmwareFeatures()
    {
      assertBarIndex(0, "Can only get firmware features from BAR 0");
      return convertToFirmwareFeatures(mBar->readRegister(Registers::FIRMWARE_FEATURES));
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

    static void setDataGeneratorEnableBits(uint32_t& bits, bool enabled)
    {
      Utilities::setBit(bits, 0, enabled);
    }

    static void setDataGeneratorRandomSizeBits(uint32_t& bits, bool enabled)
    {
      Utilities::setBit(bits, 16, enabled);
    }


//    uint8_t getDebugReadWriteRegister()
//    {
//      return mPdaBar->barRead<uint8_t>(toByteAddress(Registers::DEBUG_READ_WRITE));
//    }
//
//    void setDebugReadWriteRegister(uint8_t value)
//    {
//      return mPdaBar->barWrite<uint8_t>(toByteAddress(Registers::DEBUG_READ_WRITE), value);
//    }

  private:
    /// Convert an index to a byte address
    size_t toByteAddress(size_t address32) const
    {
      return address32 * 4;
    }

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
