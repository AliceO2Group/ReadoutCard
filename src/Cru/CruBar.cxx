// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CruBar.cxx
/// \brief Implementation of the CruBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include "CruBar.h"
#include "Gbt.h"
#include "I2c.h"
#include "Ttc.h"
#include "DatapathWrapper.h"
#include "boost/format.hpp"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {

using Link = Cru::Link;

CruBar::CruBar(const Parameters& parameters)
    : BarInterfaceBase(parameters),
      mClock(parameters.getClock().get_value_or(Clock::Local)),
      mDatapathMode(parameters.getDatapathMode().get_value_or(DatapathMode::Packet)),
      mDownstreamData(parameters.getDownstreamData().get_value_or(DownstreamData::Ctp)),
      mGbtMode(parameters.getGbtMode().get_value_or(GbtMode::Gbt)),
      mGbtMux(parameters.getGbtMux().get_value_or(GbtMux::Ttc)),
      mLinkMask(parameters.getLinkMask().get_value_or(std::set<uint32_t>{0})),
      mGbtMuxMap(parameters.getGbtMuxMap().get_value_or(std::map<uint32_t, GbtMux::type>{}))
{
  if (getIndex() == 0) {
    mFeatures = parseFirmwareFeatures(); 
  }

  if (parameters.getLinkLoopbackEnabled() == true) { // explicit true needed here
    mLoopback = 0x1;
  } else { //default to disabled
    mLoopback = 0x0;
  }
}

CruBar::CruBar(std::shared_ptr<Pda::PdaBar> bar)
    : BarInterfaceBase(bar)
{
  if (getIndex() == 0)
    mFeatures = parseFirmwareFeatures();
}
 
CruBar::~CruBar()
{
}

/*void CruBar::CruBar::checkReadSafe(int)
{
}

void CruBar::CruBar::checkWriteSafe(int, uint32_t)
{
}*/

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
  writeRegister(Cru::Registers::LINK_SUPERPAGE_ADDRESS_HIGH.get(link).index,
      Utilities::getUpper32Bits(busAddress));
  writeRegister(Cru::Registers::LINK_SUPERPAGE_ADDRESS_LOW.get(link).index,
      Utilities::getLower32Bits(busAddress));
  // Set superpage size. This write signals the push of the descriptor into the link's FIFO.
  writeRegister(Cru::Registers::LINK_SUPERPAGE_SIZE.get(link).index, pages);
}

/// Get amount of superpages pushed by a link
/// \param link Link number
uint32_t CruBar::getSuperpageCount(uint32_t link)
{
  return readRegister(Cru::Registers::LINK_SUPERPAGES_PUSHED.get(link).index);
}

/// Enables the data emulator
/// \param enabled true for enabled
void CruBar::setDataEmulatorEnabled(bool enabled)
{
  writeRegister(Cru::Registers::DMA_CONTROL.index, enabled ? 0x1 : 0x0);
  uint32_t bits = readRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index);
  setDataGeneratorEnableBits(bits, enabled);
  writeRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index, bits);
}

/// Resets the data generator counter
void CruBar::resetDataGeneratorCounter()
{
  writeRegister(Cru::Registers::RESET_CONTROL.index, 0x2);
}

/// Performs a general reset of the card
void CruBar::resetCard()
{
  writeRegister(Cru::Registers::RESET_CONTROL.index, 0x1);
}

/// Sets the pattern for the card's internal data generator
/// \param pattern Data generator pattern
/// \param size Size in bytes
/// \param randomEnabled Give true to enable random data size. In this case, the given size is the maximum size (?)
void CruBar::setDataGeneratorPattern(GeneratorPattern::type pattern, size_t size, bool randomEnabled)
{
  uint32_t bits = readRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index);
  setDataGeneratorPatternBits(bits, pattern);
  setDataGeneratorSizeBits(bits, size);
  setDataGeneratorRandomSizeBits(bits, randomEnabled);
  writeRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index, bits);
}

/// Injects a single error into the generated data stream
void CruBar::dataGeneratorInjectError()
{
  writeRegister(Cru::Registers::DATA_GENERATOR_INJECT_ERROR.index,
      Cru::Registers::DATA_GENERATOR_CONTROL_INJECT_ERROR_CMD);
}

/// Sets the data source for the DMA
void CruBar::setDataSource(uint32_t source)
{
  writeRegister(Cru::Registers::DATA_SOURCE_SELECT.index, source);
}

FirmwareFeatures CruBar::getFirmwareFeatures()
{
  return mFeatures;
}

/// Get number of dropped packets
int32_t CruBar::getDroppedPackets()
{
  return readRegister(Cru::Registers::NUM_DROPPED_PACKETS.index);
}

// Get CTP clock (Hz)
uint32_t CruBar::getCTPClock()
{
  return readRegister(Cru::Registers::CTP_CLOCK.index);
}

// Get local clock (Hz)
uint32_t CruBar::getLocalClock()
{
  return readRegister(Cru::Registers::LOCAL_CLOCK.index);
}

// Get total # of links per wrapper
int32_t CruBar::getLinks()
{
  int32_t links = 0;
  uint32_t regread = 0x0;
  regread = readRegister((Cru::Registers::WRAPPER0.address + 0x4) / 4);
  links += Utilities::getBits(regread, 24, 31);
  regread = readRegister((Cru::Registers::WRAPPER1.address + 0x4) / 4);
  links += Utilities::getBits(regread, 24, 31);

  return links;
}

// Get # of links per wrapper
int32_t CruBar::getLinksPerWrapper(int wrapper)
{
  uint32_t regread = 0x0;
  if (wrapper == 0)
    regread = readRegister((Cru::Registers::WRAPPER0.address + 0x4) / 4);
  else if (wrapper == 1)
    regread = readRegister((Cru::Registers::WRAPPER1.address + 0x4) / 4);
  else
    return 0;

  return Utilities::getBits(regread, 24, 31);

}

/// Gets the serial number from the card.
/// Note that not all firmwares have a serial number. You should make sure this firmware feature is enabled before
/// calling this function, or the card may crash. See parseFirmwareFeatures().
uint32_t CruBar::getSerialNumber()
{
  assertBarIndex(2, "Can only get serial number from BAR 2");
  auto serial = readRegister(Cru::Registers::SERIAL_NUMBER.index);
  if (serial == 0xFfffFfff) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("CRU reported invalid serial number 0xffffffff, "
          "a fatal error may have occurred"));
  }
  return serial;
}

/// Get raw data from the temperature register
uint32_t CruBar::getTemperatureRaw()
{
  assertBarIndex(2, "Can only get temperature from BAR 2");
  // Only use lower 10 bits
  return readRegister(Cru::Registers::TEMPERATURE.index) & 0x3ff;
}

/// Converts a value from the CRU's temperature register and converts it to a °C double value.
/// \param registerValue Value of the temperature register to convert
/// \return Temperature value in °C or nothing if the registerValue was invalid
boost::optional<float> CruBar::convertTemperatureRaw(uint32_t registerValue)
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
boost::optional<float> CruBar::getTemperatureCelsius()
{
  return convertTemperatureRaw(getTemperatureRaw());
}

uint32_t CruBar::getFirmwareCompileInfo()
{
  assertBarIndex(0, "Can only get firmware compile info from BAR 0");
  return readRegister(Cru::Registers::FIRMWARE_COMPILE_INFO.index);
}

uint32_t CruBar::getFirmwareGitHash()
{
  assertBarIndex(2, "Can only get git hash from BAR 2");
  return readRegister(Cru::Registers::FIRMWARE_GIT_HASH.index);
}

uint32_t CruBar::getFirmwareDateEpoch()
{
  assertBarIndex(2, "Can only get firmware epoch from BAR 2");
  return readRegister(Cru::Registers::FIRMWARE_EPOCH.index);
}

uint32_t CruBar::getFirmwareDate()
{
  assertBarIndex(2, "Can only get firmware date from BAR 2");
  return readRegister(Cru::Registers::FIRMWARE_DATE.index);
}

uint32_t CruBar::getFirmwareTime()
{
  assertBarIndex(2, "Can only get firmware time from BAR 2");
  return readRegister(Cru::Registers::FIRMWARE_TIME.index);
}

uint32_t CruBar::getFpgaChipHigh()
{
  assertBarIndex(2, "Can only get FPGA chip ID from BAR 2");
  return readRegister(Cru::Registers::FPGA_CHIP_HIGH.index);
}

uint32_t CruBar::getFpgaChipLow()
{
  assertBarIndex(2, "Can only get FPGA chip ID from BAR 2");
  return readRegister(Cru::Registers::FPGA_CHIP_LOW.index);
}

/// Get the enabled features for the card's firmware.
FirmwareFeatures CruBar::parseFirmwareFeatures()
{
  assertBarIndex(0, "Can only get firmware features from BAR 0");
  return convertToFirmwareFeatures(readRegister(Cru::Registers::FIRMWARE_FEATURES.index));
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

/// Reports the CRU status
Cru::ReportInfo CruBar::report()
{
  std::map<int, Link> linkMap = initializeLinkMap();
 
  // Update linkMap
  Gbt gbt = Gbt(mPdaBar, linkMap, mWrapperCount);
  gbt.getGbtModes();
  gbt.getGbtMuxes();
  gbt.getLoopbacks();

  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);

  for (auto& el: linkMap) {
    auto& link = el.second;
    link.datapathMode = datapathWrapper.getDatapathMode(link);
    link.enabled = datapathWrapper.getLinkEnabled(link);
  }

  Ttc ttc = Ttc(mPdaBar);
  // Mismatch between values returned by getPllClock and value required to set the clock
  // getPllClock: 0 for Local clock, 1 for TTC clock
  // setClock: 2 for Local clock, 0 for TTC clock
  uint32_t clock = (ttc.getPllClock() == 0 ? Clock::Local : Clock::Ttc);
  uint32_t downstreamData = ttc.getDownstreamData();

  Cru::ReportInfo reportInfo = {
    linkMap,
    clock,
    downstreamData
  };

  return reportInfo;
}

void CruBar::reconfigure()
{
  // Get current info
  Cru::ReportInfo reportInfo = report();

  populateLinkMap(mLinkMap);

  if (static_cast<uint32_t>(mClock) == reportInfo.ttcClock && 
      static_cast<uint32_t>(mDownstreamData) == reportInfo.downstreamData &&
      std::equal(mLinkMap.begin(), mLinkMap.end(), reportInfo.linkMap.begin())) {
    log("No need to reconfigure further");
  } else {
    log("Reconfiguring");
    configure();
  }
}

/// Configures the CRU according to the parameters passed on init
void CruBar::configure()
{
  bool ponUpstream = false;
  uint32_t onuAddress = 0xbadcafe;
  
  if (mLinkMap.empty()) {
    populateLinkMap(mLinkMap);
  }

  /* GBT */
  log("Calibrating GBT");
  Gbt gbt = Gbt(mPdaBar, mLinkMap, mWrapperCount);
  gbt.calibrateGbt();

  /* TTC */
  Ttc ttc = Ttc(mPdaBar);
  
  log("Setting the clock");
  ttc.setClock(mClock);

  log("Calibrating TTC");
  ttc.calibrateTtc();

  if (ponUpstream) {
    //ttc.resetFpll();
    if(!ttc.configurePonTx(onuAddress)) {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("PON TX fPLL phase scan failed."));
    }
  }

  log("Setting downstream data");
  ttc.selectDownstreamData(mDownstreamData);

  /* BSP */
  disableDataTaking();

  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);
  // Disable all links
  datapathWrapper.setLinksEnabled(0, 0x0);
  datapathWrapper.setLinksEnabled(1, 0x0);

  log("Enabling links and setting datapath mode");
  for (auto const& el: mLinkMap) {
    auto& link = el.second;
    if (link.enabled) {
      datapathWrapper.setLinkEnabled(link);
      datapathWrapper.setDatapathMode(link, mDatapathMode);
    }
  }

  log("Setting packet arbitration and flow control");
  datapathWrapper.setPacketArbitration(mWrapperCount, 0);
  datapathWrapper.setFlowControl(0);

  log("CRU configuration done.");
}

/// Sets the mWrapperCount variable
void CruBar::setWrapperCount()
{
  int wrapperCount = 0;

  /* Read the clock counter; If it's running increase the count */
  for (int i=0 ; i<2; i++){
    uint32_t address = Cru::getWrapperBaseAddress(i) + Cru::Registers::GBT_WRAPPER_GREGS.address + 
      Cru::Registers::GBT_WRAPPER_CLOCK_COUNTER.address;
    uint32_t reg11a = readRegister(address/4);
    uint32_t reg11b = readRegister(address/4);

    if (reg11a != reg11b) {
      wrapperCount += 1;
    }
  }
  mWrapperCount = wrapperCount;
}

/// Returns a LinkMap with indexes and base addresses initialized
std::map<int, Link> CruBar::initializeLinkMap()
{
  std::vector<Link> links;
  if (mWrapperCount == 0) {
    setWrapperCount();
  }
  for (int wrapper = 0; wrapper<mWrapperCount; wrapper++){
    uint32_t address = Cru::getWrapperBaseAddress(wrapper) + Cru::Registers::GBT_WRAPPER_CONF0.address;
    uint32_t wrapperConfig = readRegister(address/4);

    for(int bank=0; bank<6; bank++) {
      int dwrapper = (bank < 2) ? 0 : 1;
      int lpbLSB = (4 * bank) + 4;
      int lpbMSB = lpbLSB + 4 - 1;
      int linksPerBank = Utilities::getBits(wrapperConfig, lpbLSB, lpbMSB);
      if (linksPerBank == 0) {
        break;
      }
      for(int link=0; link<linksPerBank; link++) {

        int dwrapperIndex = link + (bank%2) * linksPerBank;
        int globalIndex = link + bank * linksPerBank;
        uint32_t baseAddress = Cru::getXcvrRegisterAddress(wrapper, bank, link);
        Link new_link = {
          dwrapper, 
          wrapper,
          bank,
          (uint32_t) link,
          (uint32_t) dwrapperIndex,
          (uint32_t) globalIndex,
          baseAddress,
        };

        links.push_back(new_link);
      }
    }
  }

  // Calculate "new" positions to accommodate CRU FW v3.0.0 link mapping
  std::map<int, Link> newLinkMap;
  for(int i=0; i<links.size(); i++) {
    Link link = links.at(i);
    int newPos = (i-link.bank*6) * 2 + 12*(int)(link.bank/2) + (link.bank %2);
    link.dwrapperId = newPos % 12;
    newLinkMap.insert({newPos, link});
  }


  /*for(auto el : newLinkMap)
    std::cout << el.first << " " << el.second.bank << " " << el.second.id << std::endl;*/

  return newLinkMap;
}

/// Initializes and Populates the linkMap with the GBT configuration parameters
/// Also runs the corresponding configuration
void CruBar::populateLinkMap(std::map<int, Link> &linkMap)
{
  linkMap = initializeLinkMap();

  Gbt gbt = Gbt(mPdaBar, linkMap, mWrapperCount);

  log("Configuring GBT");
  for (auto& el: linkMap) {
    auto& link = el.second;

    if (!(mLinkMask.find(el.first) == mLinkMask.end())) {
      link.enabled = true;

      gbt.setInternalDataGenerator(link, 0);
      
      link.gbtTxMode = GbtMode::Gbt;
      gbt.setTxMode(link, link.gbtTxMode); //TX is always GBT

      link.gbtRxMode = mGbtMode;
      gbt.setRxMode(link, link.gbtRxMode); //RX may also be WB

      link.loopback = mLoopback;
      gbt.setLoopback(link, link.loopback);

      auto gbtMuxMapElement = mGbtMuxMap.find(el.first);
      if (gbtMuxMapElement != mGbtMuxMap.end()) { // Different MUXs per link
        link.gbtMux = gbtMuxMapElement->second;
      } else {                                    // Specific MUX not found
        link.gbtMux = mGbtMux;
      }
      gbt.setMux(link, link.gbtMux);
      
      link.datapathMode = mDatapathMode;
    }
  }
}

int CruBar::getDdgBurstLength()
{
  uint32_t burst = (((readRegister(Cru::Registers::DDG_CTRL0.address) ) >> 20)/4 ) & 0xff;
  return Utilities::getWidth(burst);
}

void CruBar::enableDataTaking()
{
  modifyRegister(Cru::Registers::BSP_USER_CONTROL.index, 0, 1, 0x1);
}

void CruBar::disableDataTaking()
{
  modifyRegister(Cru::Registers::BSP_USER_CONTROL.index, 0, 1, 0x0);
}

void CruBar::setDebugModeEnabled(bool enabled)
{
  if (enabled) {
    writeRegister(Cru::Registers::DEBUG.index, 0x2);
  } else {
    writeRegister(Cru::Registers::DEBUG.index, 0x0);
  }
}

bool CruBar::getDebugModeEnabled()
{
  uint32_t debugMode = readRegister(Cru::Registers::DEBUG.index);
  if (debugMode == 0x0) {
    return false;
  } else {
    return true;
  }
}

int CruBar::getEndpointNumber()
{
  uint32_t endpointNumber = readRegister(Cru::Registers::ENDPOINT_ID.index);
  if (endpointNumber == 0x0) {
    return 0;
  } else if (endpointNumber == 0x11111111) {
    return 1;
  } else {
    return -1;
  }
}

} // namespace roc
} // namespace AliceO2
