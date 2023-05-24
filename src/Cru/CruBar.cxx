
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
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
#include "ReadoutCard/Logger.h"
#include "ReadoutCard/PatternPlayer.h"
#include "Utilities/Util.h"

using namespace std::literals;

namespace o2
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
    mUserLogicEnabled(parameters.getUserLogicEnabled().get_value_or(false)),
    mRunStatsEnabled(parameters.getRunStatsEnabled().get_value_or(false)),
    mUserAndCommonLogicEnabled(parameters.getUserAndCommonLogicEnabled().get_value_or(false)),
    mSystemId(parameters.getSystemId().get_value_or(0x0)),
    mFeeId(parameters.getFeeId().get_value_or(0x0)),
    mFeeIdMap(parameters.getFeeIdMap().get_value_or({})),
    mGbtPatternMode(parameters.getGbtPatternMode().get_value_or(GbtPatternMode::Counter)),
    mGbtCounterType(parameters.getGbtCounterType().get_value_or(GbtCounterType::ThirtyBit)),
    mGbtStatsMode(parameters.getGbtStatsMode().get_value_or(GbtStatsMode::All)),
    mGbtHighMask(parameters.getGbtHighMask().get_value_or(0xffffffff)),
    mGbtMedMask(parameters.getGbtMedMask().get_value_or(0xffffffff)),
    mGbtLowMask(parameters.getGbtLowMask().get_value_or(0xffffffff)),
    mTimeFrameLength(parameters.getTimeFrameLength().get_value_or(0x100))
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

  mSerial = mRocPciDevice->getSerialId().getSerial();
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

uint32_t CruBar::getSuperpageFifoEmptyCounter(uint32_t link)
{
  if (link >= Cru::MAX_LINKS) {
    BOOST_THROW_EXCEPTION(InvalidLinkId() << ErrorInfo::Message("Link ID out of range") << ErrorInfo::LinkId(link));
  }
  return readRegister(Cru::Registers::LINK_SUPERPAGE_FIFO_EMPTY.get(link).index);
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
    std::this_thread::sleep_for(40ms); // Wait some time for the I2C calls
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
      if (serialString.str().find("-") != std::string::npos) { // hack for pre production CRUs
        return 0;
      } else {
        return std::stoi(serialString.str());
      }
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

bool CruBar::getDmaStatus()
{
  mPdaBar->assertBarIndex(2, "Can only get DMA status register from BAR 2");
  return readRegister(Cru::Registers::BSP_USER_CONTROL.index) & 0x1; // DMA enabled = bit 0
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
Cru::ReportInfo CruBar::report(bool forConfig)
{
  std::map<int, Link> linkMap = initializeLinkMap();

  // strip down link map, depending on link(s) requested to report on
  // "associative-container erase idiom"
  // don't remove links for config, as they all need to be reported
  for (auto it = linkMap.cbegin(); it != linkMap.cend() && !forConfig; /* no increment */) {
    if ((mLinkMask.find(it->first) == mLinkMask.end())) {
      linkMap.erase(it++);
    } else {
      it++;
    }
  }

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
    link.enabled = datapathWrapper.isLinkEnabled(link);
    link.allowRejection = datapathWrapper.getFlowControl(link.dwrapper);
    link.stickyBit = gbt.getStickyBit(link);
    link.rxFreq = gbt.getRxClockFrequency(link) / 1e6; // Hz -> Mhz
    link.txFreq = gbt.getTxClockFrequency(link) / 1e6; // Hz -> Mhz
    link.glitchCounter = gbt.getGlitchCounter(link);
    link.fecCounter = gbt.getFecCounter(link);
    link.systemId = datapathWrapper.getSystemId(link);
    link.feeId = datapathWrapper.getFeeId(link);
    link.pktProcessed = datapathWrapper.getLinkRegister(link, Cru::Registers::DATALINK_PACKETS_PROCESSED);
    link.pktErrorProtocol = datapathWrapper.getLinkRegister(link, Cru::Registers::DATALINK_PACKETS_ERROR_PROTOCOL);
    link.pktErrorCheck1 = datapathWrapper.getLinkRegister(link, Cru::Registers::DATALINK_PACKETS_ERROR_CHECK1);
    link.pktErrorCheck2 = datapathWrapper.getLinkRegister(link, Cru::Registers::DATALINK_PACKETS_ERROR_CHECK2);
    link.pktErrorOversize = datapathWrapper.getLinkRegister(link, Cru::Registers::DATALINK_PACKETS_ERROR_OVERSIZE);
    link.orbitSor = datapathWrapper.getLinkRegister(link, Cru::Registers::DATALINK_ORBIT_SOR);

    if (link.enabled) {
      gbtEnabled = true;
    }
  }

  // Update the link map with optical power information through I2C
  I2c i2c = I2c(Cru::Registers::BSP_I2C_MINIPODS.address, 0x0, mPdaBar, mEndpoint);

  // lock I2C operations
  std::unique_ptr<Interprocess::Lock> i2cLock = std::make_unique<Interprocess::Lock>("_Alice_O2_RoC_I2C_" + std::to_string(mSerial) + "_lock", true);
  i2c.getOpticalPower(linkMap);
  i2cLock.reset();

  Ttc ttc = Ttc(mPdaBar, mSerial);
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
  bool userLogicEnabled = datapathWrapper.isLinkEnabled(userLogicLink);

  Link runStatsLink;
  runStatsLink.dwrapper = mEndpoint;
  runStatsLink.dwrapperId = (mEndpoint == 0) ? 13 : 14;
  bool runStatsEnabled = datapathWrapper.isLinkEnabled(runStatsLink);

  bool userAndCommonLogicEnabled = datapathWrapper.getUserAndCommonLogicEnabled(mEndpoint);
  uint16_t timeFrameLength = getTimeFrameLength();

  bool dmaStatus = getDmaStatus();

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
    userLogicEnabled,
    runStatsEnabled,
    userAndCommonLogicEnabled,
    timeFrameLength,
    dmaStatus
  };

  return reportInfo;
}

Cru::PacketMonitoringInfo CruBar::monitorPackets()
{
  std::map<int, Cru::LinkPacketInfo> linkPacketInfoMap;
  std::map<int, Cru::WrapperPacketInfo> wrapperPacketInfoMap;

  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);
  // Insert the Run Statistics link
  Link runStatsLink = {};
  runStatsLink.dwrapper = mEndpoint;
  runStatsLink.dwrapperId = (mEndpoint == 0) ? 13 : 14;
  uint32_t accepted = datapathWrapper.getAcceptedPackets(runStatsLink);
  uint32_t rejected = datapathWrapper.getRejectedPackets(runStatsLink);
  uint32_t forced = datapathWrapper.getForcedPackets(runStatsLink);
  linkPacketInfoMap.insert({ (int)runStatsLink.dwrapperId, { accepted, rejected, forced } });

  // Insert the UL link at 15
  Link uLLink = {};
  // dwrapper 0 on endpoint 0, dwrapper 1 on endpoint 1
  uLLink.dwrapper = mEndpoint;
  uLLink.dwrapperId = 15;
  accepted = datapathWrapper.getAcceptedPackets(uLLink);
  rejected = datapathWrapper.getRejectedPackets(uLLink);
  forced = datapathWrapper.getForcedPackets(uLLink);
  linkPacketInfoMap.insert({ 15, { accepted, rejected, forced } });

  bool userLogicEnabled = datapathWrapper.isLinkEnabled(uLLink);

  // Insert links 0-11
  std::map<int, Link> linkMap = initializeLinkMap();
  for (auto& el : linkMap) {
    // don't read the link register when UL is enabled
    uint32_t accepted = userLogicEnabled ? 0 : datapathWrapper.getAcceptedPackets(el.second);
    uint32_t rejected = userLogicEnabled ? 0 : datapathWrapper.getRejectedPackets(el.second);
    uint32_t forced = userLogicEnabled ? 0 : datapathWrapper.getForcedPackets(el.second);
    linkPacketInfoMap.insert({ el.first, { accepted, rejected, forced } });
  }

  int wrapper = mEndpoint;
  uint32_t dropped = datapathWrapper.getDroppedPackets(wrapper);
  uint32_t totalPerSecond = datapathWrapper.getTotalPacketsPerSecond(wrapper);
  wrapperPacketInfoMap.insert({ wrapper, { dropped, totalPerSecond } });

  return { linkPacketInfoMap, wrapperPacketInfoMap };
}

Cru::TriggerMonitoringInfo CruBar::monitorTriggers(bool updateable)
{
  Ttc ttc = Ttc(mPdaBar, mSerial);

  // previous values to calculate rate (every second)
  uint32_t hbCountPrev = ttc.getHbTriggerLtuCount();
  uint32_t phyCountPrev = ttc.getPhyTriggerLtuCount();
  uint32_t tofCountPrev = ttc.getTofTriggerLtuCount();
  uint32_t calCountPrev = ttc.getCalTriggerLtuCount();

  // base values to report relative counts for updateable monitoring (e.g. for a single run)
  static uint32_t hbCountBase = hbCountPrev;
  static uint32_t phyCountBase = phyCountPrev;
  static uint32_t tofCountBase = tofCountPrev;
  static uint32_t calCountBase = calCountPrev;
  static std::pair<uint32_t, uint32_t> statEoxSox = ttc.getEoxSoxLtuCount();
  static uint32_t eoxCountBase = statEoxSox.first;
  static uint32_t soxCountBase = statEoxSox.second;

  std::this_thread::sleep_for(std::chrono::seconds(1));
  uint32_t hbCount = ttc.getHbTriggerLtuCount();
  uint32_t phyCount = ttc.getPhyTriggerLtuCount();
  uint32_t tofCount = ttc.getTofTriggerLtuCount();
  uint32_t calCount = ttc.getCalTriggerLtuCount();
  std::pair<uint32_t, uint32_t> eoxSox = ttc.getEoxSoxLtuCount();
  uint32_t eoxCount = eoxSox.first;
  uint32_t soxCount = eoxSox.second;

  // current - previous counts for rates
  uint64_t hbDiff;
  if (hbCountPrev > hbCount) {
    hbDiff = hbCount + pow(2, 32) - hbCountPrev;
  } else {
    hbDiff = hbCount - hbCountPrev;
  }

  uint64_t phyDiff;
  if (phyCountPrev > phyCount) {
    phyDiff = phyCount + pow(2, 32) - phyCountPrev;
  } else {
    phyDiff = phyCount - phyCountPrev;
  }

  uint64_t tofDiff;
  if (tofCountPrev > tofCount) {
    tofDiff = tofCount + pow(2, 16) - tofCountPrev;
  } else {
    tofDiff = tofCount - tofCountPrev;
  }

  uint64_t calDiff;
  if (calCountPrev > calCount) {
    calDiff = calCount + pow(2, 16) - calCountPrev;
  } else {
    calDiff = calCount - calCountPrev;
  }

  // report absolute values + rates(1s)
  if (!updateable) {
    return { hbCount, hbDiff / pow(10, 3),
             phyCount, phyDiff / pow(10, 3),
             tofCount, tofDiff / pow(10, 3),
             calCount, calDiff / pow(10, 3),
             eoxCount, soxCount };
  }

  // current - base counts for EOX/SOX
  uint64_t eoxDiff;
  if (eoxCountBase > eoxCount) {
    eoxDiff = eoxCount + pow(2, 32) - eoxCountBase;
  } else {
    eoxDiff = eoxCount - eoxCountBase;
  }

  uint64_t soxDiff;
  if (soxCountBase > soxCount) {
    soxDiff = soxCount + pow(2, 32) - soxCountBase;
  } else {
    soxDiff = soxCount - soxCountBase;
  }

  // report relative values + rates (1s)
  return { hbCount - hbCountBase, hbDiff / pow(10, 3),
           phyCount - phyCountBase, phyDiff / pow(10, 3),
           tofCount - tofCountBase, tofDiff / pow(10, 3),
           calCount - calCountBase, calDiff / pow(10, 3),
           eoxDiff, soxDiff };
}

void CruBar::checkConfigParameters()
{
  if (mUserAndCommonLogicEnabled && !mUserLogicEnabled) {
    BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("User and Common logic switch invalid when User logic disabled"));
  }
}

/// Configures the CRU according to the parameters passed on init
void CruBar::configure(bool force)
{
  // Get current info
  Cru::ReportInfo reportInfo = report(true);
  populateLinkMap(mLinkMap);

  if (static_cast<uint32_t>(mClock) == reportInfo.ttcClock &&
      static_cast<uint32_t>(mDownstreamData) == reportInfo.downstreamData &&
      std::equal(mLinkMap.begin(), mLinkMap.end(), reportInfo.linkMap.begin()) &&
      checkPonUpstreamStatusExpected(reportInfo.ponStatusRegister, reportInfo.onuAddress) &&
      //checkClockConsistent(reportInfo.linkMap) &&
      mCruId == reportInfo.cruId &&
      mDynamicOffset == reportInfo.dynamicOffset &&
      mTriggerWindowSize == reportInfo.triggerWindowSize &&
      mUserLogicEnabled == reportInfo.userLogicEnabled &&
      mUserAndCommonLogicEnabled == reportInfo.userAndCommonLogicEnabled &&
      mRunStatsEnabled == reportInfo.runStatsEnabled &&
      mGbtEnabled == reportInfo.gbtEnabled &&
      mTimeFrameLength == reportInfo.timeFrameLength &&
      !force) {
    log("No need to reconfigure further", LogInfoDevel_(4600));
    return;
  }

  checkConfigParameters();
  log("Reconfiguring", LogInfoDevel_(4600));

  Ttc ttc = Ttc(mPdaBar, mSerial);
  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);

  /* TTC */
  if (static_cast<uint32_t>(mClock) != reportInfo.ttcClock /*|| !checkClockConsistent(reportInfo.linkMap)*/ || force) {
    log("Setting the clock to " + Clock::toString(mClock), LogInfoDevel_(4601));
    ttc.setClock(mClock);

    if (mClock == Clock::Ttc) {
      ttc.calibrateTtc();

      if (!checkPonUpstreamStatusExpected(reportInfo.ponStatusRegister, reportInfo.onuAddress) || force) {
        ttc.resetFpll();
        if (!ttc.configurePonTx(mOnuAddress)) {
          log("PON TX fPLL phase scan failed" + mOnuAddress, LogErrorDevel_(4602));
        }
      }
    }

    if (mGbtEnabled /*|| !checkClockConsistent(reportInfo.linkMap)*/) {
      Gbt gbt = Gbt(mPdaBar, mLinkMap, mWrapperCount, mEndpoint);
      gbt.calibrateGbt(mLinkMap);
      Cru::fpllref(mLinkMap, mPdaBar, 2);
      Cru::fpllcal(mLinkMap, mPdaBar);
      gbt.resetFifo();
    }
  }

  if (static_cast<uint32_t>(mDownstreamData) != reportInfo.downstreamData || force) {
    log("Setting downstream data: " + DownstreamData::toString(mDownstreamData), LogInfoDevel_(4603));
    ttc.selectDownstreamData(mDownstreamData);
  }

  /* GBT */
  if (!mGbtEnabled && ((mGbtEnabled != reportInfo.gbtEnabled) || force)) {
    // Disable all links
    datapathWrapper.setLinksEnabled(mEndpoint, 0x0);
    toggleUserLogicLink(reportInfo.userLogicEnabled); // Make sure the user logic link retains its state
    toggleRunStatsLink(reportInfo.runStatsEnabled);   // Make sure the run stats link retains its state
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

    log("System ID: " + Utilities::toHexString(mSystemId), LogInfoDevel_(4604));
    log("Allow rejection enabled: " + Utilities::toBoolString(mAllowRejection), LogInfoDevel_(4604));
    log("DatapathMode: " + DatapathMode::toString(mDatapathMode), LogInfoDevel_(4604));
    log("Enabling links:", LogInfoDevel_(4604));
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
      datapathWrapper.setSystemId(link, mSystemId);
      datapathWrapper.setFeeId(link, link.feeId);
      if (link.enabled) {
        std::stringstream linkLog;
        linkLog << "Link #" << el.first << " | GBT MUX: " << GbtMux::toString(link.gbtMux) << " | FEE ID: " << Utilities::toHexString(link.feeId);
        log(linkLog.str(), LogInfoDevel_(4604));
      }
    }
  }

  /* USER LOGIC */
  if (mUserLogicEnabled != reportInfo.userLogicEnabled || force) {
    log("User Logic enabled: " + Utilities::toBoolString(mUserLogicEnabled), LogInfoDevel_(4604));
    toggleUserLogicLink(mUserLogicEnabled);
  }

  /* RUN STATS */
  if (mRunStatsEnabled != reportInfo.runStatsEnabled || force) {
    log("Run Statistics link enabled: " + Utilities::toBoolString(mRunStatsEnabled), LogInfoDevel_(4604));
    toggleRunStatsLink(mRunStatsEnabled);
  }

  setVirtualLinksIds(mSystemId);

  /* UL + CL */
  if (mUserAndCommonLogicEnabled != reportInfo.userAndCommonLogicEnabled || force) {
    log("User and Common Logic enabled: " + Utilities::toBoolString(mUserAndCommonLogicEnabled), LogInfoDevel_(4604));
    datapathWrapper.toggleUserAndCommonLogic(mUserAndCommonLogicEnabled, mEndpoint);
  }

  /* BSP */
  if (mCruId != reportInfo.cruId || force) {
    log("Setting the CRU ID: " + Utilities::toHexString(mCruId), LogInfoDevel_(4605));
    setCruId(mCruId);
  }

  if (mTriggerWindowSize != reportInfo.triggerWindowSize || force) {
    log("Setting trigger window size: " + std::to_string(mTriggerWindowSize), LogInfoDevel_(4605));
    datapathWrapper.setTriggerWindowSize(mEndpoint, mTriggerWindowSize);
  }

  if (mDynamicOffset != reportInfo.dynamicOffset || force) {
    log("Dynamic offset enabled: " + Utilities::toBoolString(mDynamicOffset), LogInfoDevel_(4605));
    datapathWrapper.setDynamicOffset(mEndpoint, mDynamicOffset);
  }

  if (mTimeFrameLength != reportInfo.timeFrameLength || force) {
    log("Setting Time Frame length: " + std::to_string(mTimeFrameLength), LogInfoDevel_(4605));
    setTimeFrameLength(mTimeFrameLength);
  }

  log("CRU configuration done", LogInfoDevel_(4600));
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

// Returns a vector of the link ids that should participate in data taking
// A link participates in data taking if it is enabled in the datapath and is not down
std::vector<int> CruBar::getDataTakingLinks()
{
  std::vector<int> links;
  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);
  auto linkMap = initializeLinkMap();
  if (mWrapperCount == 0) {
    setWrapperCount();
  }
  Gbt gbt = Gbt(mPdaBar, linkMap, mWrapperCount, mEndpoint);

  // Insert GBT links
  for (auto const& el : linkMap) {
    int id = el.first;
    Link link = el.second;
    // TODO: Check for link UP disabled until clear what will happen with the case of TPC UL
    if (!datapathWrapper.isLinkEnabled(link) /* || gbt.getStickyBit(link) == Cru::LinkStatus::Down */) {
      continue;
    }
    links.push_back(id);
  }

  Cru::Link newLink;
  newLink.dwrapper = mEndpoint;

  // Insert Run Statistics link
  newLink.dwrapperId = (mEndpoint == 0) ? 13 : 14;
  if (datapathWrapper.isLinkEnabled(newLink)) {
    links.push_back(newLink.dwrapperId);
  }

  // Insert UL link
  newLink.dwrapperId = 15;
  if (datapathWrapper.isLinkEnabled(newLink)) {
    links.push_back(newLink.dwrapperId);
  }

  return links;
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
      link.systemId = mSystemId;

      auto feeIdMapElement = mFeeIdMap.find(el.first);
      if (feeIdMapElement != mFeeIdMap.end()) { // Different FEE IDs per link
        link.feeId = feeIdMapElement->second;
      } else { // Specific FEE ID not found
        link.feeId = mFeeId;
      }
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

void CruBar::setVirtualLinksIds(uint16_t systemId)
{
  mPdaBar->modifyRegister(Cru::Registers::VIRTUAL_LINKS_IDS.index, 16, 8, systemId);
  mPdaBar->modifyRegister(Cru::Registers::VIRTUAL_LINKS_IDS.index, 0, 16, 0x0);
}

void CruBar::emulateCtp(Cru::CtpInfo ctpInfo)
{
  Ttc ttc = Ttc(mPdaBar, mSerial);
  if (ctpInfo.generateEox) {
    log("Sending EOX", LogInfoDevel_(4800));
    ttc.setEmulatorIdleMode();
  } else if (ctpInfo.generateSingleTrigger) {
    log("Sending simple trigger", LogInfoDevel_(4801));
    ttc.doManualPhyTrigger();
  } else {
    log("Starting CTP emulator", LogInfoDevel_(4802));
    ttc.resetCtpEmulator(true);
    ttc.setEmulatorORBITINIT(ctpInfo.orbitInit);

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
      //std::vector<uint32_t> bunchCrossings = { 0x10, 0x14d, 0x29a, 0x3e7, 0x534, 0x681, 0x7ce, 0x91b, 0xa68 };
      std::vector<uint32_t> bunchCrossings = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
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

PatternPlayer::Info CruBar::patternPlayerRead()
{
  PatternPlayer pp = PatternPlayer(mPdaBar);
  return pp.read();
}

Cru::OnuStatus CruBar::reportOnuStatus(bool monitoring)
{
  Ttc ttc = Ttc(mPdaBar, mSerial, mEndpoint);
  return ttc.onuStatus(monitoring);
}

Cru::FecStatus CruBar::reportFecStatus()
{
  Ttc ttc = Ttc(mPdaBar, mSerial);
  return ttc.fecStatus();
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

void CruBar::toggleRunStatsLink(bool runStatsLinkEnabled)
{
  Link runStatsLink;
  runStatsLink.dwrapper = mEndpoint;
  runStatsLink.dwrapperId = (mEndpoint == 0) ? 13 : 14;

  DatapathWrapper datapathWrapper = DatapathWrapper(mPdaBar);
  if (runStatsLinkEnabled) {
    datapathWrapper.setLinkEnabled(runStatsLink);
  } else {
    datapathWrapper.setLinkDisabled(runStatsLink);
  }
}

boost::optional<std::string> CruBar::getUserLogicVersion()
{
  uint32_t firmwareHash = readRegister(Cru::Registers::USERLOGIC_GIT_HASH.index);
  return (boost::format("%x") % firmwareHash).str();
}

void CruBar::controlUserLogic(uint32_t eventSize, bool random, uint32_t systemId, uint32_t linkId)
{
  // reset UL
  writeRegister(Cru::Registers::USER_LOGIC_RESET.index, 0x0);

  // set event size
  writeRegister(Cru::Registers::USER_LOGIC_EVSIZE.index, eventSize);

  bool randomEventSize = readRegister(Cru::Registers::USER_LOGIC_EVSIZE_RAND.index) == 0x1;
  if (random != randomEventSize) { // toggle random evsize
    writeRegister(Cru::Registers::USER_LOGIC_EVSIZE_RAND.index, 0x1);
  }

  // set system id
  writeRegister(Cru::Registers::USER_LOGIC_SYSTEM_ID.index, systemId);

  // set link id
  writeRegister(Cru::Registers::USER_LOGIC_LINK_ID.index, linkId);
}

Cru::UserLogicInfo CruBar::reportUserLogic()
{
  bool randomEventSize = readRegister(Cru::Registers::USER_LOGIC_EVSIZE_RAND.index) == 0x1;
  uint32_t eventSize = readRegister(Cru::Registers::USER_LOGIC_EVSIZE.index);
  uint32_t systemId = readRegister(Cru::Registers::USER_LOGIC_SYSTEM_ID.index);
  uint32_t linkId = readRegister(Cru::Registers::USER_LOGIC_LINK_ID.index);
  return { eventSize, randomEventSize, systemId, linkId };
}

std::map<int, Cru::LoopbackStats> CruBar::getGbtLoopbackStats(bool reset)
{
  auto linkMap = initializeLinkMap();

  // strip down link map, depending on link(s) requested to report on
  // "associative-container erase idiom"
  for (auto it = linkMap.cbegin(); it != linkMap.cend(); /* no increment */) {
    if ((mLinkMask.find(it->first) == mLinkMask.end())) {
      linkMap.erase(it++);
    } else {
      it++;
    }
  }

  if (mWrapperCount == 0) {
    setWrapperCount();
  }
  Gbt gbt = Gbt(mPdaBar, linkMap, mWrapperCount, mEndpoint);
  return gbt.getLoopbackStats(reset, mGbtPatternMode, mGbtCounterType, mGbtStatsMode, mGbtLowMask, mGbtMedMask, mGbtHighMask);
}

uint16_t CruBar::getTimeFrameLength()
{
  // temporary hack to access the timeframe length register from bar0
  auto params = Parameters::makeParameters(SerialId{ mSerial, mEndpoint }, 0);
  auto bar0 = ChannelFactory().getBar(params);

  uint32_t timeFrameLength = bar0->readRegister(Cru::Registers::TIME_FRAME_LENGTH.index);
  return (uint16_t)Utilities::getBits(timeFrameLength, 20, 31);
}

void CruBar::setTimeFrameLength(uint16_t timeFrameLength)
{
  // temporary hack to access the timeframe length register from bar0
  auto params = Parameters::makeParameters(SerialId{ mSerial, mEndpoint }, 0);
  auto bar0 = ChannelFactory().getBar(params);

  bar0->modifyRegister(Cru::Registers::TIME_FRAME_LENGTH.index, 20, 12, timeFrameLength);
}

uint32_t CruBar::getMaxSuperpageDescriptors()
{
  return readRegister(Cru::Registers::MAX_SUPERPAGE_DESCRIPTORS.index);
}

} // namespace roc
} // namespace o2
