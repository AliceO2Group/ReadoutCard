
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
/// \file CrorcBar.cxx
/// \brief Implementation of the CrorcBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <chrono>
#include <thread>

#include "Crorc/Constants.h"
#include "Crorc/Crorc.h"
#include "Crorc/CrorcBar.h"

#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/ParameterTypes/SerialId.h"

#include "boost/format.hpp"

using namespace std::literals;

namespace o2
{
namespace roc
{

CrorcBar::CrorcBar(const Parameters& parameters, std::unique_ptr<RocPciDevice> rocPciDevice)
  : BarInterfaceBase(parameters, std::move(rocPciDevice)),
    mCrorcId(parameters.getCrorcId().get_value_or(0x0)),
    mDynamicOffset(parameters.getDynamicOffsetEnabled().get_value_or(false)),
    mLinkMask(parameters.getLinkMask().get_value_or(std::set<uint32_t>{ 0, 1, 2, 3, 4, 5 })),
    mTimeFrameLength(parameters.getTimeFrameLength().get_value_or(0x100)),
    mTimeFrameDetectionEnabled(parameters.getTimeFrameDetectionEnabled().get_value_or(true))
{
}

CrorcBar::CrorcBar(std::shared_ptr<Pda::PdaBar> bar)
  : BarInterfaceBase(bar)
{
}

CrorcBar::~CrorcBar()
{
}

boost::optional<std::string> CrorcBar::getFirmwareInfo()
{
  uint32_t fwHash = readRegister(Crorc::Registers::FIRMWARE_HASH.index);
  return (boost::format("%x") % fwHash).str();
}

boost::optional<int32_t> CrorcBar::getSerial()
{
  return getSerialNumber();
}

void CrorcBar::setSerial(int serial)
{
  Crorc::setSerial(*(mPdaBar.get()), serial);
}

boost::optional<int32_t> CrorcBar::getSerialNumber()
{
  //  mPdaBar->assertBarIndex(0, "Can only get serial from Bar 0");
  uint32_t serial = readRegister(Crorc::Registers::SERIAL_NUMBER.index);
  if (serial == 0x0) {
    writeRegister(Crorc::Registers::SERIAL_NUMBER_CTRL.index, Crorc::Registers::SERIAL_NUMBER_TRG);
    std::this_thread::sleep_for(500ms);
    serial = readRegister(Crorc::Registers::SERIAL_NUMBER.index);
  }

  if (serial == 0x0) { // Previous S/N scheme
    return Crorc::getSerial(*(mPdaBar.get()));
  } else {
    // Register format e.g. 0x32343932
    // transltes to -> 2942
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

int CrorcBar::getEndpointNumber()
{
  return 0;
}

void CrorcBar::configure(bool /*force*/)
{
  log("Configuring...", LogInfoDevel_(4650));

  // enable laser
  log("Enabling the laser", LogInfoDevel_(4651));
  setQsfpEnabled();

  log("Configuring fixed/dynamic offset", LogInfoDevel_(4652));
  // choose between fixed and dynamic offset
  setDynamicOffsetEnabled(mDynamicOffset);

  // set crorc id
  log("Setting the CRORC ID", LogInfoDevel_(4653));
  setCrorcId(mCrorcId);

  // set timeframe length
  log("Setting the Time Frame length", LogInfoDevel_(4654));
  setTimeFrameLength(mTimeFrameLength);

  log("Configuring Time Frame detection", LogInfoDevel_(4655));
  setTimeFrameDetectionEnabled(mTimeFrameDetectionEnabled);

  log("CRORC configuration done", LogInfoDevel_(4656));
}

Crorc::ReportInfo CrorcBar::report(bool forConfig)
{
  std::map<int, Crorc::Link> linkMap = initializeLinkMap();

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

  getOpticalPowers(linkMap);

  for (auto& el : linkMap) {
    auto& link = el.second;
    link.orbitSor = readRegister(Crorc::Registers::ORBIT_SOR.index);
  }

  Crorc::ReportInfo reportInfo = {
    linkMap,
    getCrorcId(),
    getTimeFrameLength(),
    getTimeFrameDetectionEnabled(),
    getQsfpEnabled(),
    getDynamicOffsetEnabled(),
  };
  return reportInfo;
}

std::map<int, Crorc::Link> CrorcBar::initializeLinkMap()
{
  std::map<int, Crorc::Link> linkMap;
  for (int bar = 0; bar < 6; bar++) {
    Crorc::Link newLink = { isLinkUp(bar) ? Crorc::LinkStatus::Up : Crorc::LinkStatus::Down, 0.0 };
    linkMap.insert({ bar, newLink });
  }
  return linkMap;
}

bool CrorcBar::checkLinkUp()
{
  return !((mPdaBar->readRegister(Crorc::Registers::CHANNEL_CSR.index) & Crorc::Registers::LINK_DOWN));
}

void CrorcBar::assertLinkUp()
{
  int checkTimes = 1000;
  auto timeOut = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
  while ((std::chrono::steady_clock::now() < timeOut) && checkTimes) {
    if (checkLinkUp()) {
      checkTimes--;
    } else {
      checkTimes = 1000;
    }
  }

  if (!checkLinkUp()) {
    BOOST_THROW_EXCEPTION(CrorcCheckLinkException() << ErrorInfo::Message(getLoggerPrefix() + "Link was not up"));
  }
}

bool CrorcBar::isLinkUp(int barIndex)
{
  auto params = Parameters::makeParameters(SerialId{ getSerial().get(), getEndpointNumber() }, barIndex); // TODO: This is problematic...
  auto bar = ChannelFactory().getBar(params);
  return !((bar->readRegister(Crorc::Registers::CHANNEL_CSR.index) & Crorc::Registers::LINK_DOWN));
}

void CrorcBar::setQsfpEnabled()
{
  uint32_t qsfpStatus = (readRegister(Crorc::Registers::LINK_STATUS.index) >> 31) & 0x1;
  if (qsfpStatus == 0) {
    writeRegister(Crorc::Registers::I2C_CMD.index, 0x80);
  }
}

bool CrorcBar::getQsfpEnabled()
{
  return (readRegister(Crorc::Registers::LINK_STATUS.index) >> 31) & 0x1;
}

void CrorcBar::setCrorcId(uint16_t crorcId)
{
  modifyRegister(Crorc::Registers::CFG_CONTROL.index, 4, 12, crorcId);
}

uint16_t CrorcBar::getCrorcId()
{
  return (readRegister(Crorc::Registers::CFG_CONTROL.index) >> 4) & 0x0fff;
}

void CrorcBar::setDynamicOffsetEnabled(bool enabled)
{
  modifyRegister(Crorc::Registers::CFG_CONTROL.index, 0, 1, enabled ? 0x1 : 0x0);
}

bool CrorcBar::getDynamicOffsetEnabled()
{
  return (readRegister(Crorc::Registers::CFG_CONTROL.index) & 0x1);
}

void CrorcBar::setTimeFrameLength(uint16_t timeFrameLength)
{
  if (timeFrameLength > 256) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(getLoggerPrefix() + "BAD TF LENGTH, should be less or equal to 256") << ErrorInfo::ConfigValue(timeFrameLength));
  }
  modifyRegister(Crorc::Registers::CFG_CONTROL_B.index, 0, 11, timeFrameLength);
}

uint16_t CrorcBar::getTimeFrameLength()
{
  return readRegister(Crorc::Registers::CFG_CONTROL_B.index) & 0x0fff;
}

void CrorcBar::setTimeFrameDetectionEnabled(bool enabled)
{
  modifyRegister(Crorc::Registers::CFG_CONTROL_B.index, 12, 1, enabled ? 0x1 : 0x0);
}

bool CrorcBar::getTimeFrameDetectionEnabled()
{
  return (readRegister(Crorc::Registers::CFG_CONTROL_B.index) >> 12) & 0x1;
}

void CrorcBar::flushSuperpages()
{
  modifyRegister(Crorc::Registers::CFG_CONTROL_B.index, 16, 1, 0x1);
}

void CrorcBar::getOpticalPowers(std::map<int, Crorc::Link>& linkMap)
{
  for (auto& el : linkMap) {
    auto linkNo = el.first;
    auto& link = el.second;

    uint32_t opt;

    if (linkNo < 2) {
      opt = readRegister(Crorc::Registers::OPT_POWER_QSFP10.index);
    } else if (linkNo < 4) {
      opt = readRegister(Crorc::Registers::OPT_POWER_QSFP32.index);
    } else {
      opt = readRegister(Crorc::Registers::OPT_POWER_QSFP54.index);
    }
    if (linkNo % 2 == 0) {
      opt &= 0xffff;
    } else {
      opt = (opt & 0xffff0000) >> 16;
    }
    link.opticalPower = opt / 10;
  }
}

void CrorcBar::sendDdlCommand(uint32_t address, uint32_t command)
{
  writeRegister(address / 4, command);

  auto checkFifoEmpty = [&]() {
    return readRegister(Crorc::Registers::CHANNEL_CSR.index) & Crorc::Registers::RXSTAT_NOT_EMPTY;
  };

  auto timeOut = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
  while ((std::chrono::steady_clock::now() < timeOut) && !checkFifoEmpty()) {
  }
  if (!checkFifoEmpty()) {
    BOOST_THROW_EXCEPTION(CrorcCheckLinkException() << ErrorInfo::Message(getLoggerPrefix() + "Link was not up"));
  } else {
    readRegister(Crorc::Registers::DDL_STATUS.index);
  }
}

void CrorcBar::resetSiu()
{
  sendDdlCommand(Crorc::Registers::DDL_COMMAND.address, Crorc::Registers::SIU_RESET);
}

void CrorcBar::startTrigger(uint32_t command) //TODO: Rename this
{
  sendDdlCommand(Crorc::Registers::DDL_COMMAND.address, command);
}

void CrorcBar::stopTrigger()
{
  try {
    sendDdlCommand(Crorc::Registers::DDL_COMMAND.address, Crorc::Registers::EOBTR);
  } catch (const Exception& e) {
    log("Stopping DDL trigger timed out", LogInfoDevel_(4656));
  }
}

void CrorcBar::resetCard()
{
  writeRegister(Crorc::Registers::CRORC_CSR.index, Crorc::Registers::CRORC_RESET);
}

void CrorcBar::resetDevice(bool withSiu)
{
  resetCard();
  if (withSiu) {
    resetSiu();
    assertLinkUp();
  }

  if (withSiu) {
    resetSiu();
  }
  resetCard();
  if (withSiu) {
    assertLinkUp();
  }
}

void CrorcBar::startDataReceiver(uintptr_t superpageInfoBusAddress)
{
  // Send the address of the DMA buffer used for writing the Superpage Info (size + count)
  writeRegister(Crorc::Registers::SPINFO_LOW.index, (superpageInfoBusAddress & 0xffffffff));
  if (sizeof(uintptr_t) > 4) {
    writeRegister(Crorc::Registers::SPINFO_HIGH.index, (superpageInfoBusAddress >> 32));
  } else {
    writeRegister(Crorc::Registers::SPINFO_HIGH.index, 0x0);
  }

  // Set DMA ON
  uint32_t CSRRegister = readRegister(Crorc::Registers::CHANNEL_CSR.index);
  if (!Utilities::getBit(CSRRegister, Crorc::Registers::DATA_RX_ON_OFF_BIT)) {
    Utilities::setBit(CSRRegister, Crorc::Registers::DATA_RX_ON_OFF_BIT, true);
    writeRegister(Crorc::Registers::CHANNEL_CSR.index, CSRRegister);
  }
}

void CrorcBar::stopDataReceiver()
{
  uint32_t CSRRegister = readRegister(Crorc::Registers::CHANNEL_CSR.index);
  if (Utilities::getBit(CSRRegister, Crorc::Registers::DATA_RX_ON_OFF_BIT)) {
    // Implemented as a toggle, so we write the same value we did for starting
    Utilities::setBit(CSRRegister, Crorc::Registers::DATA_RX_ON_OFF_BIT, true);
    writeRegister(Crorc::Registers::CHANNEL_CSR.index, CSRRegister);
  }

  nSPpush = 0;
  nSPpushErr = 0;
  nSPcounter = 0;
}

void CrorcBar::pushSuperpageAddressAndSize(uintptr_t blockAddress, uint32_t blockLength)
{
/*
  writeRegister(Crorc::Registers::SP_WR_ADDR_HIGH.index, arch64() ? (blockAddress >> 32) : 0x0);
  writeRegister(Crorc::Registers::SP_WR_ADDR_LOW.index, blockAddress & 0xffffffff);
  writeRegister(Crorc::Registers::SP_WR_SIZE.index, blockLength);
*/

  uint32_t vwr; // value to write
  uint32_t vxp; // value expected on read back (not always = vwr)
  uint32_t vrd; // value read back
  size_t ix; // address index

  auto checkSuccess = [&] () {
    if (vrd != vxp) {
      char err[1024];
      snprintf(err,1024, "pushSuperpageAddress : write failed ix = 0x%X write = 0x%X expected = 0x%X != read 0x%X", (int)ix, (int)vwr, (int)vxp, (int)vrd);
      log(err, LogWarningDevel_(4699));
      nSPpushErr++;
      return -1;
    }
    return 0;
  };

  try {
    nSPpush++;

    ix = Crorc::Registers::SP_WR_ADDR_HIGH.index;
    vxp = vwr = arch64() ? (blockAddress >> 32) : 0x0;
    writeRegister(ix, vwr);
    vrd = readRegister(ix);
    checkSuccess();

    ix = Crorc::Registers::SP_WR_ADDR_LOW.index;
    vxp = vwr = blockAddress & 0xffffffff;
    writeRegister(ix, vwr);
    vrd = readRegister(ix);
    checkSuccess();

    ix = Crorc::Registers::SP_WR_SIZE.index;
    vxp = vwr = blockLength;
    vxp = (vwr << 8) | (++nSPcounter); // weird feature of crorc
    writeRegister(ix, vwr);
    vrd = readRegister(ix);
    if (!checkSuccess()) {
      nSPcounter = vrd & 0xFF;
    }
  } catch(...) {
    nSPpushErr++;
    log("pushSuperpageAddress exception", LogErrorDevel_(4699));
  }
  return;
}

void CrorcBar::startDataGenerator()
{
  modifyRegister(Crorc::Registers::DATA_GENERATOR_CFG.index, 31, 1, 0x1);
}

void CrorcBar::stopDataGenerator()
{
  modifyRegister(Crorc::Registers::DATA_GENERATOR_CFG.index, 31, 1, 0x0);
}

void CrorcBar::setLoopback()
{
  if ((readRegister(Crorc::Registers::CHANNEL_CSR.index) & Crorc::Registers::LOOPBACK_ON_OFF) == 0x0) {
    writeRegister(Crorc::Registers::CHANNEL_CSR.index, Crorc::Registers::LOOPBACK_ON_OFF);
  }
}

void CrorcBar::setDiuLoopback()
{
  sendDdlCommand(Crorc::Registers::DDL_COMMAND.address, Crorc::Registers::DIU_LOOPBACK);
  //  sendDdlCommand(Crorc::Registers::DDL_COMMAND.address, Crorc::Registers::DIU_RANDCIFST);
}

void CrorcBar::setSiuLoopback()
{
  sendDdlCommand(Crorc::Registers::DDL_COMMAND.address, Crorc::Registers::SIU_LOOPBACK);
  //  sendDdlCommand(Crorc::Registers::DDL_COMMAND.address, Crorc::Registers::SIU_RANDCIFST);
}

Crorc::PacketMonitoringInfo CrorcBar::monitorPackets()
{
  uint32_t acquisitionRate = readRegister(Crorc::Registers::ACQ_RATE.index);
  uint32_t packetsReceived = readRegister(Crorc::Registers::PKTS_RECEIVED.index);
  return { acquisitionRate, packetsReceived };
}

} // namespace roc
} // namespace o2
