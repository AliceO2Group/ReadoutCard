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

#include "Crorc/Constants.h"
#include "Crorc/Crorc.h"
#include "Crorc/CrorcBar.h"

#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/ParameterTypes/SerialId.h"

#include "boost/format.hpp"

namespace AliceO2
{
namespace roc
{

CrorcBar::CrorcBar(const Parameters& parameters, std::unique_ptr<RocPciDevice> rocPciDevice)
  : BarInterfaceBase(parameters, std::move(rocPciDevice)),
    mCrorcId(parameters.getCrorcId().get_value_or(0x0)),
    mDynamicOffset(parameters.getDynamicOffsetEnabled().get_value_or(false))
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
  return Crorc::getSerial(*(mPdaBar.get()));
}

void CrorcBar::setSerial(int serial)
{
  Crorc::setSerial(*(mPdaBar.get()), serial);
}

int CrorcBar::getEndpointNumber()
{
  return 0;
}

void CrorcBar::configure(bool /*force*/)
{
  // enable laser
  log("Enabling the laser");
  setQsfpEnabled();

  log("Configuring fixed/dynamic offset");
  // choose between fixed and dynamic offset
  setDynamicOffsetEnabled(mDynamicOffset);

  // set crorc id
  log("Setting the CRORC ID");
  setCrorcId(mCrorcId);
}

Crorc::ReportInfo CrorcBar::report()
{
  std::map<int, Crorc::Link> linkMap = initializeLinkMap();
  getOpticalPowers(linkMap);
  Crorc::ReportInfo reportInfo = {
    linkMap,
    getCrorcId(),
    getQsfpEnabled(),
    getDynamicOffsetEnabled()
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

bool CrorcBar::isLinkUp(int barIndex)
{
  auto params = Parameters::makeParameters(SerialId{ getSerial().get(), getEndpointNumber() }, barIndex);
  auto bar = ChannelFactory().getBar(params);
  return !((bar->readRegister(Crorc::Registers::C_CSR.index) & Crorc::Registers::LINK_DOWN));
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

void CrorcBar::getOpticalPowers(std::map<int, Crorc::Link>& linkMap)
{
  for (auto& el : linkMap) {
    auto linkNo = el.first;
    auto& link = el.second;
    if (linkNo < 4) {
      link.opticalPower = readRegister(Crorc::Registers::OPT_POWER_QSFP0.get(linkNo).index) & 0xffff;
    } else {
      link.opticalPower = readRegister(Crorc::Registers::OPT_POWER_QSFP1.get(linkNo % 4).index) & 0xffff;
    }
    link.opticalPower /= 10;
  }
}

} // namespace roc
} // namespace AliceO2
