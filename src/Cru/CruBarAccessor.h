/// \file CruBarAccessor.h
/// \brief Definition of the CruRegisterAccessor class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstddef>
#include <boost/optional/optional.hpp>
#include "CruRegisterIndex.h"
#include "Pda/PdaBar.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace Rorc {

/// A simple wrapper object for accessing the CRU BAR
class CruBarAccessor
{
  public:
    CruBarAccessor(Pda::PdaBar* pdaBar)
        : mPdaBar(pdaBar)
    {
    }

    enum class BufferStatus
    {
        AVAILABLE,
        BUSY
    };

    /// Set the registers of a descriptor entry
    /// \param index FIFO index
    /// \param pages Amount of 8 kiB pages in superpage
    /// \param address Superpage PCI bus address
    void setSuperpageDescriptor(uint32_t index, uint32_t pages, uintptr_t busAddress)
    {
      assert(index < FIFO_INDEXES);

      printf("Setting superpage descriptor i=%u pages=%u address=0x%lx\n", index, pages, busAddress);

      // Set superpage address
      mPdaBar->barWrite(SUPERPAGE_ADDRESS_HIGH, Utilities::getUpper32Bits(busAddress));
      mPdaBar->barWrite(SUPERPAGE_ADDRESS_LOW, Utilities::getLower32Bits(busAddress));

      // Set superpage size and FIFO index
      uint32_t pagesAvailableAndIndex = index & 0xf;
      pagesAvailableAndIndex |= (pages << 4) & (~0xf);
      mPdaBar->barWrite(SUPERPAGE_PAGES_AVAILABLE_AND_INDEX, pagesAvailableAndIndex);

      // Set superpage enabled
      mPdaBar->barWrite(SUPERPAGE_STATUS, index);
    }

    uint32_t getSuperpagePushedPages(uint32_t index)
    {
      assert(index < FIFO_INDEXES);
      return mPdaBar->barRead<uint32_t>(SUPERPAGE_PUSHED_PAGES + index*4);
    }

    BufferStatus getSuperpageBufferStatus(uint32_t index)
    {
      uint32_t status = mPdaBar->barRead<uint32_t>(SUPERPAGE_STATUS);
      uint32_t bit = status & (0b1 << index);
      if (bit == 0) {
        return BufferStatus::BUSY;
      } else {
        return BufferStatus::AVAILABLE;
      }
    }

    void setDataEmulatorEnabled(bool enabled) const
    {
      at32(CruRegisterIndex::DATA_EMULATOR_CONTROL) = enabled ? 0x3 : 0x0;
    }

    void resetDataGeneratorCounter() const
    {
      at32(CruRegisterIndex::RESET_CONTROL) = 0x2;
    }

    void resetCard() const
    {
      at32(CruRegisterIndex::RESET_CONTROL) = 0x1;
    }

    void setFifoBusAddress(void* address) const
    {
      at32(CruRegisterIndex::STATUS_BASE_BUS_HIGH) = Utilities::getUpper32Bits(uint64_t(address));
      at32(CruRegisterIndex::STATUS_BASE_BUS_LOW) = Utilities::getLower32Bits(uint64_t(address));
    }

    void setFifoCardAddress() const
    {
      at32(CruRegisterIndex::STATUS_BASE_CARD_HIGH) = 0x0;
      at32(CruRegisterIndex::STATUS_BASE_CARD_LOW) = 0x8000;
    }

    void setDescriptorTableSize() const
    {
      at32(CruRegisterIndex::DESCRIPTOR_TABLE_SIZE) = 127; // NUM_PAGES - 1;
    }

    void setDoneControl() const
    {
      at32(CruRegisterIndex::DONE_CONTROL) = 0x1;
    }

    void setDataGeneratorPattern(GeneratorPattern::type pattern)
    {
      constexpr uint32_t INCREMENTAL = 0b01;
      constexpr uint32_t ALTERNATING = 0b10;
      constexpr uint32_t CONSTANT = 0b11;

      auto value = [&pattern]{ switch (pattern) {
          case GeneratorPattern::Incremental: return INCREMENTAL;
          case GeneratorPattern::Alternating: return ALTERNATING;
          case GeneratorPattern::Constant:    return CONSTANT;
          default: BOOST_THROW_EXCEPTION(Exception()
              << ErrorInfo::Message("Unsupported generator pattern for CRU")
              << ErrorInfo::GeneratorPattern(pattern)); }};

      at32(CruRegisterIndex::DMA_CONFIGURATION) = value();
    }

    uint32_t getSerialNumber() const
    {
      if (mPdaBar->getBarNumber() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Can only get serial number from BAR 2")
            << ErrorInfo::BarIndex(mPdaBar->getBarNumber()));
      }

      return at32(CruRegisterIndex::SERIAL_NUMBER);
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
      return at32(CruRegisterIndex::TEMPERATURE) & 0x3ff;
    }

    /// Converts a value from the CRU's temperature register and converts it to a °C double value.
    /// \param registerValue Value of the temperature register to convert
    /// \return Temperature value in °C or nothing if the registerValue was invalid
    boost::optional<float> convertTemperatureRaw(uint32_t registerValue) const
    {
      /// It's a 10 bit register, so: 2^10 - 1
      constexpr int REGISTER_MAX_VALUE = 1023;

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

    uint32_t getIdleCounterLower()
    {
      return at32(CruRegisterIndex::IDLE_COUNTER_LOWER);
    }

    uint32_t getIdleCounterUpper()
    {
      return at32(CruRegisterIndex::IDLE_COUNTER_UPPER);
    }

    uint64_t getIdleCounter()
    {
      return (uint64_t(getIdleCounterUpper()) << 32) + uint64_t(getIdleCounterLower());
    }

    uint32_t getIdleMaxValue()
    {
      return at32(CruRegisterIndex::MAX_IDLE_VALUE);
    }

    uint32_t getFirmwareCompileInfo()
    {
      return at32(CruRegisterIndex::FIRMWARE_COMPILE_INFO);
    }

    uint8_t getDebugReadWriteRegister()
    {
      return mPdaBar->getRegister<uint8_t>(CruRegisterIndex::toByteAddress(CruRegisterIndex::DEBUG_READ_WRITE));
    }

    void setDebugReadWriteRegister(uint8_t value)
    {
      return mPdaBar->setRegister<uint8_t>(CruRegisterIndex::toByteAddress(CruRegisterIndex::DEBUG_READ_WRITE), value);
    }

    void setLedState(bool onOrOff)
    {
      int on = 0x00; // Yes, a 0 represents the on state
      int off = 0xff;
      at32(CruRegisterIndex::LED_STATUS) = onOrOff ? on : off;
    }

  private:

    // These are for the new interface prototype. Highly subject to change.
    // For more info: https://alice.its.cern.ch/jira/browse/CRU-61
    static constexpr int SUPERPAGE_ADDRESS_HIGH = 0x210;
    static constexpr int SUPERPAGE_ADDRESS_LOW = 0x214;
    static constexpr int SUPERPAGE_PAGES_AVAILABLE_AND_INDEX = 0x234;
    static constexpr int SUPERPAGE_STATUS = 0x23c;
    static constexpr int SUPERPAGE_PUSHED_PAGES = 0x240;
    static constexpr int FIFO_INDEXES = 4;

    volatile uint32_t& at32(size_t index) const
    {
      return mPdaBar->at<uint32_t>(CruRegisterIndex::toByteAddress(index));
    }

    const Pda::PdaBar* mPdaBar;
//    uintptr_t bar;
};

} // namespace Rorc
} // namespace AliceO2
