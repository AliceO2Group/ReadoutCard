/// \file CruBar.cxx
/// \brief Implementation of the CruBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "CruBar.h"
#include "boost/format.hpp"

namespace AliceO2 {
namespace roc {

CruBar::CruBar(const Parameters& parameters)
    : BarInterfaceBase(parameters)
{
  if (mPdaBar->getIndex() == 0)
    mFeatures = parseFirmwareFeatures();
}
CruBar::CruBar(std::shared_ptr<Pda::PdaBar> bar)
    : BarInterfaceBase(bar)
{
  if (mPdaBar->getIndex() == 0)
    mFeatures = parseFirmwareFeatures();
} 
 
CruBar::~CruBar()
{
}

void CruBar::CruBar::checkReadSafe(int)
{
}

void CruBar::CruBar::checkWriteSafe(int, uint32_t)
{
}

boost::optional<int32_t> CruBar::getSerial()
{
  return getSerialNumber();
}

boost::optional<float> CruBar::getTemperature()
{
  return getTemperatureCelsius();
}

boost::optional<std::string> CruBar::getFirmwareInfo()
{
  return (boost::format("%x-%x-%x") % getFirmwareDate() % getFirmwareTime()
      % getFirmwareGitHash()).str();
}

boost::optional<std::string> CruBar::getCardId()
{
  return (boost::format("%08x-%08x") % getFpgaChipHigh() % getFpgaChipLow()).str();
}

/// Push a superpage into the FIFO of a link
/// \param link Link number
/// \param pages Amount of 8 kiB pages in superpage
/// \param busAddress Superpage PCI bus address
void CruBar::pushSuperpageDescriptor(uint32_t link, uint32_t pages, uintptr_t busAddress)
{
  // Set superpage address. These writes are buffered on the firmware side.
  mPdaBar->writeRegister(Cru::Registers::LINK_SUPERPAGE_ADDRESS_HIGH.get(link).index,
      Utilities::getUpper32Bits(busAddress));
  mPdaBar->writeRegister(Cru::Registers::LINK_SUPERPAGE_ADDRESS_LOW.get(link).index,
      Utilities::getLower32Bits(busAddress));
  // Set superpage size. This write signals the push of the descriptor into the link's FIFO.
  mPdaBar->writeRegister(Cru::Registers::LINK_SUPERPAGE_SIZE.get(link).index, pages);
}

/// Get amount of superpages pushed by a link
/// \param link Link number
uint32_t CruBar::getSuperpageCount(uint32_t link)
{
  return mPdaBar->readRegister(Cru::Registers::LINK_SUPERPAGES_PUSHED.get(link).index);
}

/// Enables the data emulator
/// \param enabled true for enabled
void CruBar::setDataEmulatorEnabled(bool enabled) const
{
  mPdaBar->writeRegister(Cru::Registers::DMA_CONTROL.index, enabled ? 0x1 : 0x0);
  uint32_t bits = mPdaBar->readRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index);
  setDataGeneratorEnableBits(bits, enabled);
  mPdaBar->writeRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index, bits);
}

/// Resets the data generator counter
void CruBar::resetDataGeneratorCounter() const
{
  mPdaBar->writeRegister(Cru::Registers::RESET_CONTROL.index, 0x2);
}

/// Performs a general reset of the card
void CruBar::resetCard() const
{
  mPdaBar->writeRegister(Cru::Registers::RESET_CONTROL.index, 0x1);
}

/// Sets the pattern for the card's internal data generator
/// \param pattern Data generator pattern
/// \param size Size in bytes
/// \param randomEnabled Give true to enable random data size. In this case, the given size is the maximum size (?)
void CruBar::setDataGeneratorPattern(GeneratorPattern::type pattern, size_t size, bool randomEnabled)
{
  uint32_t bits = mPdaBar->readRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index);
  setDataGeneratorPatternBits(bits, pattern);
  setDataGeneratorSizeBits(bits, size);
  setDataGeneratorRandomSizeBits(bits, randomEnabled);
  mPdaBar->writeRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index, bits);
}

/// Injects a single error into the generated data stream
void CruBar::dataGeneratorInjectError()
{
  mPdaBar->writeRegister(Cru::Registers::DATA_GENERATOR_INJECT_ERROR.index,
      Cru::Registers::DATA_GENERATOR_CONTROL_INJECT_ERROR_CMD);
}

/// Sets the data source for the DMA
void CruBar::setDataSource(uint32_t source)
{
  mPdaBar->writeRegister(Cru::Registers::DATA_SOURCE_SELECT.index, source);
}

/// Set links
void CruBar::setLinksEnabled(uint32_t mask)
{
  mPdaBar->writeRegister(Cru::Registers::LINKS_ENABLE.index, mask);
}

FirmwareFeatures CruBar::getFirmwareFeatures()
{
  return mFeatures;
}

/// Gets the serial number from the card.
/// Note that not all firmwares have a serial number. You should make sure this firmware feature is enabled before
/// calling this function, or the card may crash. See parseFirmwareFeatures().
uint32_t CruBar::getSerialNumber() const
{
  assertBarIndex(2, "Can only get serial number from BAR 2");
  auto serial = mPdaBar->readRegister(Cru::Registers::SERIAL_NUMBER.index);
  if (serial == 0xFfffFfff) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("CRU reported invalid serial number 0xffffffff, "
          "a fatal error may have occurred"));
  }
  return serial;
}

/// Get raw data from the temperature register
uint32_t CruBar::getTemperatureRaw() const
{
  assertBarIndex(2, "Can only get temperature from BAR 2");
  // Only use lower 10 bits
  return mPdaBar->readRegister(Cru::Registers::TEMPERATURE.index) & 0x3ff;
}

/// Converts a value from the CRU's temperature register and converts it to a °C double value.
/// \param registerValue Value of the temperature register to convert
/// \return Temperature value in °C or nothing if the registerValue was invalid
boost::optional<float> CruBar::convertTemperatureRaw(uint32_t registerValue) const
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
boost::optional<float> CruBar::getTemperatureCelsius() const
{
  return convertTemperatureRaw(getTemperatureRaw());
}

uint32_t CruBar::getFirmwareCompileInfo()
{
  assertBarIndex(0, "Can only get firmware compile info from BAR 0");
  return mPdaBar->readRegister(Cru::Registers::FIRMWARE_COMPILE_INFO.index);
}

uint32_t CruBar::getFirmwareGitHash()
{
  assertBarIndex(2, "Can only get git hash from BAR 2");
  return mPdaBar->readRegister(Cru::Registers::FIRMWARE_GIT_HASH.index);
}

uint32_t CruBar::getFirmwareDateEpoch()
{
  assertBarIndex(2, "Can only get firmware epoch from BAR 2");
  return mPdaBar->readRegister(Cru::Registers::FIRMWARE_EPOCH.index);
}

uint32_t CruBar::getFirmwareDate()
{
  assertBarIndex(2, "Can only get firmware date from BAR 2");
  return mPdaBar->readRegister(Cru::Registers::FIRMWARE_DATE.index);
}

uint32_t CruBar::getFirmwareTime()
{
  assertBarIndex(2, "Can only get firmware time from BAR 2");
  return mPdaBar->readRegister(Cru::Registers::FIRMWARE_TIME.index);
}

uint32_t CruBar::getFpgaChipHigh()
{
  assertBarIndex(2, "Can only get FPGA chip ID from BAR 2");
  return mPdaBar->readRegister(Cru::Registers::FPGA_CHIP_HIGH.index);
}

uint32_t CruBar::getFpgaChipLow()
{
  assertBarIndex(2, "Can only get FPGA chip ID from BAR 2");
  return mPdaBar->readRegister(Cru::Registers::FPGA_CHIP_LOW.index);
}

/// Get the enabled features for the card's firmware.
FirmwareFeatures CruBar::parseFirmwareFeatures()
{
  assertBarIndex(0, "Can only get firmware features from BAR 0");
  return convertToFirmwareFeatures(mPdaBar->readRegister(Cru::Registers::FIRMWARE_FEATURES.index));
}

FirmwareFeatures CruBar::convertToFirmwareFeatures(uint32_t reg)
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
    features.chipId = false;
  } else {
    // Integrated firmware
    features.standalone = false;
    features.temperature = true;
    features.dataSelection = true;
    features.serial = true;
    features.firmwareInfo = true;
    features.chipId = true;
  }
  return features;
}

/// Sets the bits for the data generator pattern in the given integer
/// \param bits Integer with bits to set
/// \param pattern Pattern to set
void CruBar::setDataGeneratorPatternBits(uint32_t& bits, GeneratorPattern::type pattern)
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
void CruBar::setDataGeneratorSizeBits(uint32_t& bits, size_t size)
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
void CruBar::setDataGeneratorEnableBits(uint32_t& bits, bool enabled)
{
  Utilities::setBit(bits, 0, enabled);
}

/// Sets the bits for the data generator random size in the given integer
/// \param bits Integer with bits to set
/// \param enabled Random enabled or not
void CruBar::setDataGeneratorRandomSizeBits(uint32_t& bits, bool enabled)
{
  Utilities::setBit(bits, 16, enabled);
}

} // namespace roc
} // namespace AliceO2
