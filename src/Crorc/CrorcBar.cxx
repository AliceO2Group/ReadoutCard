// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

namespace AliceO2
{
namespace roc
{

CrorcBar::CrorcBar(const Parameters& parameters, std::unique_ptr<RocPciDevice> rocPciDevice)
  : BarInterfaceBase(parameters, std::move(rocPciDevice)),
    mCrorcId(parameters.getCrorcId().get_value_or(0x0)),
    mDynamicOffset(parameters.getDynamicOffsetEnabled().get_value_or(false)),
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
  log("Configuring...", LogInfoOps);

  // enable laser
  log("Enabling the laser", LogInfoDevel);
  setQsfpEnabled();

  log("Configuring fixed/dynamic offset", LogInfoDevel);
  // choose between fixed and dynamic offset
  setDynamicOffsetEnabled(mDynamicOffset);

  // set crorc id
  log("Setting the CRORC ID", LogInfoDevel);
  setCrorcId(mCrorcId);

  // set timeframe length
  log("Setting the Time Frame length", LogInfoDevel);
  setTimeFrameLength(mTimeFrameLength);

  log("Configuring Time Frame detection", LogInfoDevel);
  setTimeFrameDetectionEnabled(mTimeFrameDetectionEnabled);

  log("CRORC configuration done", LogInfoOps);
}

Crorc::ReportInfo CrorcBar::report()
{
  std::map<int, Crorc::Link> linkMap = initializeLinkMap();
  getOpticalPowers(linkMap);
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
    BOOST_THROW_EXCEPTION(CrorcCheckLinkException() << ErrorInfo::Message("Link was not up"));
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
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD TF LENGTH, should be less or equal to 256") << ErrorInfo::ConfigValue(timeFrameLength));
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
    BOOST_THROW_EXCEPTION(CrorcCheckLinkException() << ErrorInfo::Message("Link was not up"));
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
    log("Stopping DDL trigger timed out");
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
  }
  assertLinkUp();

  if (withSiu) {
    resetSiu();
  }
  resetCard();
  assertLinkUp();
}

void CrorcBar::startDataReceiver(uintptr_t readyFifoBusAddress)
{
  writeRegister(Crorc::Registers::CHANNEL_RRBAR.index, (readyFifoBusAddress & 0xffffffff));
  if (sizeof(uintptr_t) > 4) {
    writeRegister(Crorc::Registers::CHANNEL_RRBARX.index, (readyFifoBusAddress >> 32));
  } else {
    writeRegister(Crorc::Registers::CHANNEL_RRBARX.index, 0x0);
  }

  if (!(readRegister(Crorc::Registers::CHANNEL_CSR.index) & Crorc::Registers::DATA_RX_ON_OFF)) {
    writeRegister(Crorc::Registers::CHANNEL_CSR.index, Crorc::Registers::DATA_RX_ON_OFF);
  }
}

void CrorcBar::stopDataReceiver()
{
  if (readRegister(Crorc::Registers::CHANNEL_CSR.index) & Crorc::Registers::DATA_RX_ON_OFF) {
    writeRegister(Crorc::Registers::CHANNEL_CSR.index, Crorc::Registers::DATA_RX_ON_OFF);
  }
}

void CrorcBar::pushRxFreeFifo(uintptr_t blockAddress, uint32_t blockLength, uint32_t readyFifoIndex)
{
  writeRegister(Crorc::Registers::RX_FIFO_ADDR_EXT.index, arch64() ? (blockAddress >> 32) : 0x0);
  writeRegister(Crorc::Registers::RX_FIFO_ADDR_HIGH.index, blockAddress & 0xffffffff);
  writeRegister(Crorc::Registers::RX_FIFO_ADDR_LOW.index, (blockLength << 8) | readyFifoIndex);
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
} // namespace AliceO2
