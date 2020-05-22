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
#include "Eeprom.h"
#include "Gbt.h"
#include "I2c.h"
#include "Ttc.h"
#include "DatapathWrapper.h"
#include "boost/format.hpp"
#include "ReadoutCard/PatternPlayer.h"
#include "Utilities/Util.h"

using namespace std::literals;

namespace AliceO2
{
namespace roc
{

using Link = Cru::Link;

CruBar::CruBar(const Parameters& parameters, std::unique_ptr<RocPciDevice> rocPciDevice)
  : BarInterfaceBase(parameters, std::move(rocPciDevice)),
    mClock(parameters.getClock().get_value_or(Clock::Local)),
    mCruId(parameters.getCruId().get_value_or(0x0)),
    mDatapathMode(parameters.getDatapathMode().get_value_or(DatapathMode::Packet)),
    mDownstreamData(parameters.getDownstreamData().get_value_or(DownstreamData::Ctp)),
    mGbtMode(parameters.getGbtMode().get_value_or(GbtMode::Gbt)),
    mGbtMux(parameters.getGbtMux().get_value_or(GbtMux::Ttc)),
    mLinkMask(parameters.getLinkMask().get_value_or(std::set<uint32_t>{ 0 })),
    mGbtMuxMap(parameters.getGbtMuxMap().get_value_or(std::map<uint32_t, GbtMux::type>{})),
    mPonUpstream(parameters.getPonUpstreamEnabled().get_value_or(false)),
    mOnuAddress(parameters.getOnuAddress().get_value_or(0x0)),
    mDynamicOffset(parameters.getDynamicOffsetEnabled().get_value_or(false)),
    mTriggerWindowSize(parameters.getTriggerWindowSize().get_value_or(1000)),
    mGbtEnabled(parameters.getGbtEnabled().get_value_or(true)),
    mUserLogicEnabled(parameters.getUserLogicEnabled().get_value_or(false))
{
  if (getIndex() == 0) {
    mFeatures = parseFirmwareFeatures();
  }

  if (parameters.getLinkLoopbackEnabled() == true) { // explicit true needed here
    mLoopback = 0x1;
  } else { //default to disabled
    mLoopback = 0x0;
  }

  if (parameters.getAllowRejection() == true) {
    mAllowRejection = 0x1;
  } else {
    mAllowRejection = 0x0;
  }

  mEndpoint = mRocPciDevice->getSerialId().getEndpoint();
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
  //return (boost::format("%x-%x-%x") % getFirmwareDate() % getFirmwareTime() % getFirmwareGitHash()).str();
  return (boost::format("%x") % getFirmwareGitHash()).str();
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
  writeRegister(Cru::Registers::LINK_SUPERPAGE_PAGES.get(link).index, pages);
}

/// Get amount of superpages pushed by a link
/// \param link Link number
uint32_t CruBar::getSuperpageCount(uint32_t link)
{
  return readRegister(Cru::Registers::LINK_SUPERPAGE_COUNT.get(link).index);
}

uint32_t CruBar::getSuperpageSize(uint32_t link)
{
  writeRegister(Cru::Registers::LINK_SUPERPAGE_SIZE.get(link).index, 0xbadcafe); // write a dummy value to update the FIFO
  uint32_t superpageSizeFifo = readRegister(Cru::Registers::LINK_SUPERPAGE_SIZE.get(link).index);
  uint32_t superpageSize = Utilities::getBits(superpageSizeFifo, 0, 23); // [0-23] -> superpage size (in bytes)
  if (superpageSize == 0) {                                              // No reason to check for index -> superpageSize == 0 -> CRU FW < v3.4.0
    return 0;
  }
  uint32_t superpageSizeIndex = Utilities::getBits(superpageSizeFifo, 24, 31); // [24-31] -> superpage index (0-255)

  while (superpageSizeIndex != mSuperpageSizeIndexCounter[link]) { // In case the PCIe bus wasn't fast enough
    superpageSizeFifo = readRegister(Cru::Registers::LINK_SUPERPAGE_SIZE.get(link).index);
    superpageSize = Utilities::getBits(superpageSizeFifo, 0, 23);
    superpageSizeIndex = Utilities::getBits(superpageSizeFifo, 24, 31);
  }

  mSuperpageSizeIndexCounter[link] = (superpageSizeIndex + 1) % 256;

  return superpageSize;
}

/// Signals the CRU DMA engine to start
void CruBar::startDmaEngine()
{
  writeRegister(Cru::Registers::DMA_CONTROL.index, 0x11);                  // send DMA start (bit #0)
                                                                           // dyn offset enabled (bit #4)
  modifyRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index, 0, 1, 0x1); // enable data generator
}

/// Signals the CRU DMA engine to stop
void CruBar::stopDmaEngine()
{
  modifyRegister(Cru::Registers::DMA_CONTROL.index, 8, 1, 0x1);            // send DMA flush to the CRU
  modifyRegister(Cru::Registers::DATA_GENERATOR_CONTROL.index, 0, 1, 0x0); // disable data generator
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

/// Resets internal counters
void CruBar::resetInternalCounters()
{
  // clear internal superpage size index counter array
  std::fill(mSuperpageSizeIndexCounter, mSuperpageSizeIndexCounter + Cru::MAX_LINKS, 0);
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
uint32_t CruBar::getDroppedPackets(int endpoint)
{
  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);
  return datapathWrapper.getDroppedPackets(endpoint);
}

uint32_t CruBar::getTotalPacketsPerSecond(int endpoint)
{
  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);
  return datapathWrapper.getTotalPacketsPerSecond(endpoint);
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
boost::optional<int32_t> CruBar::getSerialNumber()
{
  mPdaBar->assertBarIndex(2, "Can only get serial number from BAR 2");
  uint32_t serial = readRegister(Cru::Registers::SERIAL_NUMBER.index);
  if (serial == 0x0) { // Try to populate the serial register in case it's empty
    writeRegister(Cru::Registers::SERIAL_NUMBER_CTRL.index, Cru::Registers::SERIAL_NUMBER_TRG);
    std::this_thread::sleep_for(5ms); // Wait some time for the I2C calls
    serial = readRegister(Cru::Registers::SERIAL_NUMBER.index);
  }

  if (serial == 0x0) { // Pre v3.6.3 scheme; we need to support it for now
    Eeprom eeprom = Eeprom(mPdaBar);
    boost::optional<int32_t> serial = eeprom.getSerial();
    return serial;
  } else { // v3.6.3+
    // Register format e.g. 0x35343230
    // translates to -> 0245
    std::stringstream serialString;

    serialString << char(serial & 0xff)
                 << char((serial & 0xff00) >> 8)
                 << char((serial & 0xff0000) >> 16)
                 << char((serial & 0xff000000) >> 24);

    try {
      return std::stoi(serialString.str());
    } catch (...) {
      return {};
    }
  }
}

/// Get raw data from the temperature register
uint32_t CruBar::getTemperatureRaw()
{
  mPdaBar->assertBarIndex(2, "Can only get temperature from BAR 2");
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
  mPdaBar->assertBarIndex(0, "Can only get firmware compile info from BAR 0");
  return readRegister(Cru::Registers::FIRMWARE_COMPILE_INFO.index);
}

uint32_t CruBar::getFirmwareGitHash()
{
  mPdaBar->assertBarIndex(2, "Can only get git hash from BAR 2");
  return readRegister(Cru::Registers::FIRMWARE_GIT_HASH.index);
}

uint32_t CruBar::getFirmwareDateEpoch()
{
  mPdaBar->assertBarIndex(2, "Can only get firmware epoch from BAR 2");
  return readRegister(Cru::Registers::FIRMWARE_EPOCH.index);
}

uint32_t CruBar::getFirmwareDate()
{
  mPdaBar->assertBarIndex(2, "Can only get firmware date from BAR 2");
  return readRegister(Cru::Registers::FIRMWARE_DATE.index);
}

uint32_t CruBar::getFirmwareTime()
{
  mPdaBar->assertBarIndex(2, "Can only get firmware time from BAR 2");
  return readRegister(Cru::Registers::FIRMWARE_TIME.index);
}

uint32_t CruBar::getFpgaChipHigh()
{
  mPdaBar->assertBarIndex(2, "Can only get FPGA chip ID from BAR 2");
  return readRegister(Cru::Registers::FPGA_CHIP_HIGH.index);
}

uint32_t CruBar::getFpgaChipLow()
{
  mPdaBar->assertBarIndex(2, "Can only get FPGA chip ID from BAR 2");
  return readRegister(Cru::Registers::FPGA_CHIP_LOW.index);
}

uint32_t CruBar::getPonStatusRegister()
{
  mPdaBar->assertBarIndex(2, "Can only get PON status register from BAR 2");
  return readRegister((Cru::Registers::ONU_USER_LOGIC.address + 0x0c) / 4);
}

uint32_t CruBar::getOnuAddress()
{
  mPdaBar->assertBarIndex(2, "Can only get PON status register from BAR 2");
  return readRegister(Cru::Registers::ONU_USER_LOGIC.index) >> 1;
}

bool CruBar::checkPonUpstreamStatusExpected(uint32_t ponUpstreamRegister, uint32_t onuAddress)
{
  if (!mPonUpstream) { //no need to check if ponUpstream is disabled
    return true;
  } else if ((ponUpstreamRegister == 0xff || ponUpstreamRegister == 0xf7) && (mOnuAddress == onuAddress)) {
    //ponUpstream should be 0b11110111 or 0b11111111
    //onuAddress should be the same as requested
    return true;
  } else {
    return false;
  }
}

bool CruBar::checkClockConsistent(std::map<int, Link> linkMap)
{
  float prevTxFreq = -1;
  for (auto& el : linkMap) {
    auto& link = el.second;
    if (prevTxFreq != link.txFreq && prevTxFreq != -1) {
      return false;
    }
    prevTxFreq = link.txFreq;
  }
  return true;
}

/// Get the enabled features for the card's firmware.
FirmwareFeatures CruBar::parseFirmwareFeatures()
{
  mPdaBar->assertBarIndex(0, "Can only get firmware features from BAR 0");
  return convertToFirmwareFeatures(readRegister(Cru::Registers::FIRMWARE_FEATURES.index));
}

FirmwareFeatures CruBar::convertToFirmwareFeatures(uint32_t reg)
{
  FirmwareFeatures features;
  uint32_t safeword = Utilities::getBits(reg, 0, 15);
  if (safeword == 0x5afe) {
    // Standalone firmware
    auto enabled = [&](int i) { return Utilities::getBit(reg, i) == 0; };
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

/// Reports the CRU status
Cru::ReportInfo CruBar::report()
{
  std::map<int, Link> linkMap = initializeLinkMap();

  bool gbtEnabled = false;
  // Update linkMap
  Gbt gbt = Gbt(mPdaBar, linkMap, mWrapperCount, mEndpoint);
  gbt.getGbtModes();
  gbt.getGbtMuxes();
  gbt.getLoopbacks();

  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);

  for (auto& el : linkMap) {
    auto& link = el.second;
    link.datapathMode = datapathWrapper.getDatapathMode(link);
    link.enabled = datapathWrapper.getLinkEnabled(link);
    link.allowRejection = datapathWrapper.getFlowControl(link.dwrapper);
    link.stickyBit = gbt.getStickyBit(link);
    link.rxFreq = gbt.getRxClockFrequency(link) / 1e6; // Hz -> Mhz
    link.txFreq = gbt.getTxClockFrequency(link) / 1e6; // Hz -> Mhz

    if (link.enabled) {
      gbtEnabled = true;
    }
  }

  // Update the link map with optical power information through I2C
  I2c i2c = I2c(Cru::Registers::BSP_I2C_MINIPODS.address, 0x0, mPdaBar, mEndpoint);
  i2c.getOpticalPower(linkMap);

  Ttc ttc = Ttc(mPdaBar);
  // Mismatch between values returned by getPllClock and value required to set the clock
  // getPllClock: 0 for Local clock, 1 for TTC clock
  // setClock: 2 for Local clock, 0 for TTC clock
  uint32_t clock = (ttc.getPllClock() == 0 ? Clock::Local : Clock::Ttc);
  uint32_t downstreamData = ttc.getDownstreamData();
  uint32_t ponStatusRegister = getPonStatusRegister();
  uint32_t onuAddress = getOnuAddress();
  uint16_t cruId = getCruId();
  bool dynamicOffset = datapathWrapper.getDynamicOffsetEnabled(mEndpoint);
  uint32_t triggerWindowSize = datapathWrapper.getTriggerWindowSize(mEndpoint);

  Link userLogicLink;
  userLogicLink.dwrapper = mEndpoint;
  userLogicLink.dwrapperId = 15;

  bool userLogicEnabled = datapathWrapper.getLinkEnabled(userLogicLink);

  Cru::ReportInfo reportInfo = {
    linkMap,
    clock,
    downstreamData,
    ponStatusRegister,
    onuAddress,
    cruId,
    dynamicOffset,
    triggerWindowSize,
    gbtEnabled,
    userLogicEnabled
  };

  return reportInfo;
}

Cru::PacketMonitoringInfo CruBar::monitorPackets()
{
  std::map<int, Cru::LinkPacketInfo> linkPacketInfoMap;
  std::map<int, Cru::WrapperPacketInfo> wrapperPacketInfoMap;

  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);
  std::map<int, Link> linkMap = initializeLinkMap();
  for (auto& el : linkMap) {
    uint32_t accepted = datapathWrapper.getAcceptedPackets(el.second);
    uint32_t rejected = datapathWrapper.getRejectedPackets(el.second);
    uint32_t forced = datapathWrapper.getForcedPackets(el.second);
    linkPacketInfoMap.insert({ el.first, { accepted, rejected, forced } });
  }

  // Insert the UL link at 15
  Link uLLink = {};
  // dwrapper 0 on endpoint 0, dwrapper 1 on endpoint 1
  uLLink.dwrapper = mEndpoint;
  uLLink.dwrapperId = 15;
  uint32_t accepted = datapathWrapper.getAcceptedPackets(uLLink);
  uint32_t rejected = datapathWrapper.getRejectedPackets(uLLink);
  uint32_t forced = datapathWrapper.getForcedPackets(uLLink);
  linkPacketInfoMap.insert({ 15, { accepted, rejected, forced } });

  int wrapper = mEndpoint;
  uint32_t dropped = datapathWrapper.getDroppedPackets(wrapper);
  uint32_t totalPerSecond = datapathWrapper.getTotalPacketsPerSecond(wrapper);
  wrapperPacketInfoMap.insert({ wrapper, { dropped, totalPerSecond } });

  return { linkPacketInfoMap, wrapperPacketInfoMap };
}

/// Configures the CRU according to the parameters passed on init
void CruBar::configure(bool force)
{
  // Get current info
  Cru::ReportInfo reportInfo = report();
  populateLinkMap(mLinkMap);

  if (static_cast<uint32_t>(mClock) == reportInfo.ttcClock &&
      static_cast<uint32_t>(mDownstreamData) == reportInfo.downstreamData &&
      std::equal(mLinkMap.begin(), mLinkMap.end(), reportInfo.linkMap.begin()) &&
      checkPonUpstreamStatusExpected(reportInfo.ponStatusRegister, reportInfo.onuAddress) &&
      checkClockConsistent(reportInfo.linkMap) &&
      mCruId == reportInfo.cruId &&
      mDynamicOffset == reportInfo.dynamicOffset &&
      mTriggerWindowSize == reportInfo.triggerWindowSize &&
      mUserLogicEnabled == reportInfo.userLogicEnabled &&
      mGbtEnabled == reportInfo.gbtEnabled &&
      !force) {
    log("No need to reconfigure further");
    return;
  }

  log("Reconfiguring");

  Ttc ttc = Ttc(mPdaBar);
  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);

  /* TTC */
  if (static_cast<uint32_t>(mClock) != reportInfo.ttcClock || !checkClockConsistent(reportInfo.linkMap) || force) {
    log("Setting the clock");
    ttc.setClock(mClock);

    log("Calibrating TTC");
    if (mClock == Clock::Ttc) {
      ttc.calibrateTtc();

      if (!checkPonUpstreamStatusExpected(reportInfo.ponStatusRegister, reportInfo.onuAddress) || force) {
        ttc.resetFpll();
        if (!ttc.configurePonTx(mOnuAddress)) {
          log("PON TX fPLL phase scan failed", InfoLogger::InfoLogger::Error);
        }
      }
    }

    if (mGbtEnabled || !checkClockConsistent(reportInfo.linkMap)) {
      log("Calibrating the fPLLs");
      Gbt gbt = Gbt(mPdaBar, mLinkMap, mWrapperCount, mEndpoint);
      gbt.calibrateGbt(mLinkMap);
      Cru::fpllref(mLinkMap, mPdaBar, 2);
      Cru::fpllcal(mLinkMap, mPdaBar);
    }
  }

  if (static_cast<uint32_t>(mDownstreamData) != reportInfo.downstreamData || force) {
    log("Setting downstream data");
    ttc.selectDownstreamData(mDownstreamData);
  }

  /* GBT */
  if (!mGbtEnabled && ((mGbtEnabled != reportInfo.gbtEnabled) || force)) {
    // Disable all links
    datapathWrapper.setLinksEnabled(mEndpoint, 0x0);
    toggleUserLogicLink(reportInfo.userLogicEnabled); // Make sure the user logic link retains its state
  } else if (mGbtEnabled && (!std::equal(mLinkMap.begin(), mLinkMap.end(), reportInfo.linkMap.begin()) || force)) {
    /* BSP */
    disableDataTaking();

    // Disable DWRAPPER datagenerator (in case of restart)
    datapathWrapper.resetDataGeneratorPulse();
    datapathWrapper.useDataGeneratorSource(false);
    datapathWrapper.enableDataGenerator(false);
    // Disable all links
    //datapathWrapper.setLinksEnabled(0, 0x0);
    //datapathWrapper.setLinksEnabled(1, 0x0);

    log("Enabling links and setting datapath mode and flow control");
    for (auto const& el : mLinkMap) {
      auto& link = el.second;
      auto& linkPrevState = reportInfo.linkMap.at(el.first);
      if (linkPrevState != link || force) {
        // link mismatch
        // -> toggle enabled status
        if (link.enabled != linkPrevState.enabled) {
          // toggle enable/disable
          if (linkPrevState.enabled) {
            datapathWrapper.setLinkDisabled(link);
          } else {
            datapathWrapper.setLinkEnabled(link);
          }
        }
        datapathWrapper.setDatapathMode(link, mDatapathMode);
      }
      datapathWrapper.setFlowControl(link.dwrapper, mAllowRejection); //Set flow control anyway as it's per dwrapper
    }
  }

  /* USER LOGIC */
  if (mUserLogicEnabled != reportInfo.userLogicEnabled || force) {
    log("Toggling the User Logic link");
    toggleUserLogicLink(mUserLogicEnabled);
  }

  /* BSP */
  if (mCruId != reportInfo.cruId || force) {
    log("Setting the CRU ID");
    setCruId(mCruId);
  }

  if (mTriggerWindowSize != reportInfo.triggerWindowSize || force) {
    log("Setting trigger window size");
    datapathWrapper.setTriggerWindowSize(mEndpoint, mTriggerWindowSize);
  }

  if (mDynamicOffset != reportInfo.dynamicOffset || force) {
    log("Toggling fixed/dynamic offset");
    datapathWrapper.setDynamicOffset(mEndpoint, mDynamicOffset);
  }

  log("CRU configuration done.");
}

/// Sets the mWrapperCount variable
void CruBar::setWrapperCount()
{
  int wrapperCount = 0;

  /* Read the clock counter; If it's running increase the count */
  for (int i = 0; i < 2; i++) {
    uint32_t address = Cru::getWrapperBaseAddress(i) + Cru::Registers::GBT_WRAPPER_GREGS.address +
                       Cru::Registers::GBT_WRAPPER_CLOCK_COUNTER.address;
    uint32_t reg11a = readRegister(address / 4);
    uint32_t reg11b = readRegister(address / 4);

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
  for (int wrapper = 0; wrapper < mWrapperCount; wrapper++) {
    uint32_t address = Cru::getWrapperBaseAddress(wrapper) + Cru::Registers::GBT_WRAPPER_CONF0.address;
    uint32_t wrapperConfig = readRegister(address / 4);

    for (int bank = mEndpoint * 2; bank < (mEndpoint * 2 + 4); bank++) {
      //for (int bank = 0; bank < 4; bank++) { //for 0-23 range
      // endpoint 0 -> banks {0,1}
      // endpoint 1 -> banks {2,3}
      int dwrapper = (bank < 2) ? 0 : 1;
      int lpbLSB = (4 * bank) + 4;
      int lpbMSB = lpbLSB + 4 - 1;
      int linksPerBank = Utilities::getBits(wrapperConfig, lpbLSB, lpbMSB);
      if (linksPerBank == 0) {
        break;
      }
      for (int link = 0; link < linksPerBank; link++) {

        int dwrapperIndex = link + (bank % 2) * linksPerBank;
        int globalIndex = link + bank * linksPerBank;
        uint32_t baseAddress = Cru::getXcvrRegisterAddress(wrapper, bank, link);
        Link new_link = {
          dwrapper,
          wrapper,
          bank,
          (uint32_t)link,
          (uint32_t)dwrapperIndex,
          (uint32_t)globalIndex,
          baseAddress,
        };

        links.push_back(new_link);
      }
    }
  }

  // Calculate "new" positions to accommodate CRU FW v3.0.0 link mapping
  std::map<int, Link> newLinkMap;
  for (std::size_t i = 0; i < links.size(); i++) {
    Link link = links.at(i);
    int newPos = (i - (link.bank - mEndpoint * 2) * 6) * 2 + (link.bank % 2);
    //int newPos = (i - link.bank * 6) * 2 + + 12 * (int)(link.bank/2) + (link.bank % 2); //for 0-23 range
    link.dwrapperId = newPos % 12;
    newLinkMap.insert({ newPos, link });
  }

  /*for(auto el : newLinkMap)
    std::cout << el.first << " " << el.second.bank << " " << el.second.id << std::endl;*/

  return newLinkMap;
}

/// Initializes and Populates the linkMap with the GBT configuration parameters
/// Also runs the corresponding configuration
void CruBar::populateLinkMap(std::map<int, Link>& linkMap)
{
  linkMap = initializeLinkMap();

  Gbt gbt = Gbt(mPdaBar, linkMap, mWrapperCount, mEndpoint);

  for (auto& el : linkMap) {
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
      } else { // Specific MUX not found
        link.gbtMux = mGbtMux;
      }
      gbt.setMux(el.first, link.gbtMux);

      link.datapathMode = mDatapathMode;
      link.allowRejection = mAllowRejection;
    } else {
      link.loopback = false; // disabled links should NOT be in loopback
      gbt.setLoopback(link, link.loopback);
    }
  }
}

uint32_t CruBar::getDdgBurstLength()
{
  uint32_t burst = (((readRegister(Cru::Registers::DDG_CTRL0.address)) >> 20) / 4) & 0xff;
  return burst;
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

void CruBar::setCruId(uint16_t cruId)
{
  modifyRegister(Cru::Registers::BSP_USER_CONTROL.index, 16, 12, cruId);
}

uint16_t CruBar::getCruId()
{
  return (readRegister(Cru::Registers::BSP_USER_CONTROL.index) >> 16) & 0x0fff;
}

void CruBar::emulateCtp(Cru::CtpInfo ctpInfo)
{
  Ttc ttc = Ttc(mPdaBar);
  if (ctpInfo.generateEox) {
    ttc.setEmulatorIdleMode();
  } else if (ctpInfo.generateSingleTrigger) {
    ttc.doManualPhyTrigger();
  } else {
    ttc.resetCtpEmulator(true);

    if (ctpInfo.triggerMode == Cru::TriggerMode::Periodic) {
      ttc.setEmulatorPHYSDIV(ctpInfo.triggerFrequency);
      ttc.setEmulatorHCDIV(5);
      ttc.setEmulatorCALDIV(5);
    } else if (ctpInfo.triggerMode == Cru::TriggerMode::Hc) {
      ctpInfo.triggerMode = Cru::TriggerMode::Periodic;
      ttc.setEmulatorPHYSDIV(5);
      ttc.setEmulatorHCDIV(ctpInfo.triggerFrequency);
      ttc.setEmulatorCALDIV(5);
    } else if (ctpInfo.triggerMode == Cru::TriggerMode::Cal) {
      ctpInfo.triggerMode = Cru::TriggerMode::Periodic;
      ttc.setEmulatorPHYSDIV(5);
      ttc.setEmulatorHCDIV(5);
      ttc.setEmulatorCALDIV(ctpInfo.triggerFrequency);
    } else if (ctpInfo.triggerMode == Cru::TriggerMode::Fixed) {
      ctpInfo.triggerMode = Cru::TriggerMode::Periodic;

      // Don't send PHYS continuously (no PHY trigger if rate < 7)
      ttc.setEmulatorPHYSDIV(5);
      std::vector<uint32_t> bunchCrossings = { 0x10, 0x14d, 0x29a, 0x3e7, 0x534, 0x681, 0x7ce, 0x91b, 0xa68 };
      ttc.setFixedBCTrigger(bunchCrossings);
    }

    ttc.setEmulatorTriggerMode(ctpInfo.triggerMode);

    ttc.setEmulatorBCMAX(ctpInfo.bcMax);
    ttc.setEmulatorHBMAX(ctpInfo.hbMax);
    ttc.setEmulatorPrescaler(ctpInfo.hbKeep, ctpInfo.hbDrop);

    //ttc.setEmulatorHCDIV(38); // to generate periodic hc triggers
    //ttc.setEmulatorCALDIV(5); // to generate periodic cal triggers

    ttc.resetCtpEmulator(false);
  }
}

void CruBar::patternPlayer(PatternPlayer::Info info)
{
  PatternPlayer pp = PatternPlayer(mPdaBar);
  pp.play(info);
}

Cru::OnuStatus CruBar::reportOnuStatus()
{
  Ttc ttc = Ttc(mPdaBar);
  return ttc.onuStatus();
}

void CruBar::toggleUserLogicLink(bool userLogicEnabled)
{
  Link userLogicLink;
  userLogicLink.dwrapper = mEndpoint;
  userLogicLink.dwrapperId = 15;

  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);
  if (userLogicEnabled) {
    datapathWrapper.setLinkEnabled(userLogicLink);
  } else {
    datapathWrapper.setLinkDisabled(userLogicLink);
  }
  datapathWrapper.setDatapathMode(userLogicLink, mDatapathMode);
}

boost::optional<std::string> CruBar::getUserLogicVersion()
{
  uint32_t firmwareHash = readRegister(Cru::Registers::USERLOGIC_GIT_HASH.index);
  return (boost::format("%x") % firmwareHash).str();
}

} // namespace roc
} // namespace AliceO2
