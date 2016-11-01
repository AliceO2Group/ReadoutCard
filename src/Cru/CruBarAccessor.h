/// \file CruBarAccessor.h
/// \brief Definition of the CruRegisterAccessor class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstddef>
#include "CruRegisterIndex.h"
#include "Pda/PdaBar.h"
#include "Util.h"

namespace AliceO2 {
namespace Rorc {

/// A simple wrapper object for accessing the CRU BAR
class CruBarAccessor
{
  public:
    CruBarAccessor(Pda::PdaBar& pdaBar)
        : mPdaBar(pdaBar)
    {
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
      // TODO see if this works with BAR uint64_t setter
      at32(CruRegisterIndex::STATUS_BASE_BUS_HIGH) = Util::getUpper32Bits(uint64_t(address));
      at32(CruRegisterIndex::STATUS_BASE_BUS_LOW) = Util::getLower32Bits(uint64_t(address));
    }

    [[deprecated]] void setFifoCardAddress() const
    {
      at32(CruRegisterIndex::STATUS_BASE_CARD_HIGH) = 0x0;
      at32(CruRegisterIndex::STATUS_BASE_CARD_LOW) = 0x8000;
    }

    [[deprecated]] void setDescriptorTableSize() const
    {
      at32(CruRegisterIndex::DESCRIPTOR_TABLE_SIZE) = 127; // NUM_PAGES - 1;
    }

    [[deprecated]] void setDoneControl() const
    {
      at32(CruRegisterIndex::DONE_CONTROL) = 0x1;
    }

    void sendAcknowledge() const
    {
      at32(CruRegisterIndex::DMA_COMMAND) = 0x1;
    }

    uint32_t getSerialNumber() const
    {
      if (mPdaBar.getBarNumber() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << errinfo_rorc_error_message("Can only get serial number from BAR 2")
            << errinfo_rorc_bar_index(mPdaBar.getBarNumber()));
      }
      return at32(CruRegisterIndex::SERIAL_NUMBER);
    }

    /// Get raw data from the temperature register
    uint32_t getTemperatureRaw() const
    {
      return at32(CruRegisterIndex::TEMPERATURE);
    }

    /// Converts a value from the CRU's temperature register and converts it to a °C double value.
    /// \param registerValue Value of the temperature register to convert
    /// \return Temperature value in °C or nothing if the registerValue was invalid
    boost::optional<double> convertTemperatureRaw(uint32_t registerValue) const
    {
      /// It's a 10 bit register, so: 2^10 - 1
      constexpr int REGISTER_MAX_VALUE = 1023;

      // Conversion formula from: https://documentation.altera.com/#/00045071-AA$AA00044865
      if (registerValue == 0 || registerValue > REGISTER_MAX_VALUE) {
        return boost::none;
      } else {
        double A = 693.0;
        double B = 265.0;
        double C = double(registerValue);
        return ((A * C) / 1024.0) - B;
      }
    }

    /// Gets the temperature in °C, or nothing if the temperature value was invalid.
    /// \param registerValue Value of the temperature register to convert
    /// \return Temperature value in °C or nothing if the temperature value was invalid
    boost::optional<double> getTemperatureCelsius() const
    {
      return convertTemperatureRaw(getTemperatureRaw());
    }

  private:
    volatile uint32_t& at32(size_t index) const
    {
      return mPdaBar.at<uint32_t>(CruRegisterIndex::toByteAddress(index));
    }

    const Pda::PdaBar& mPdaBar;
};

} // namespace Rorc
} // namespace AliceO2
