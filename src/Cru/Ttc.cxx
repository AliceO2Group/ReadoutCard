
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
/// \file Ttc.cxx
/// \brief Implementation of the Ttc class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <thread>
#include "Constants.h"
#include "I2c.h"
#include "Ttc.h"
#include "Utilities/Util.h"
#include "register_maps/Si5345-RevD_local_pll1_zdb-Registers.h"
#include "register_maps/Si5345-RevD_local_pll2_zdb-Registers.h"
#include "register_maps/Si5345-RevD_ttc_pll1_zdb-Registers.h"
#include "register_maps/Si5345-RevD_ttc_pll2_zdb-Registers.h"
#include "register_maps/Si5344-RevD-TFC_40-Registers.h"
#include "register_maps/Si5345-RevD-_local_pll-oct-2024-320M-LPGBTCRU-Registers.h"

namespace o2
{
namespace roc
{

using LinkStatus = Cru::LinkStatus;
using OnuStickyStatus = Cru::OnuStickyStatus;
using OnuStatus = Cru::OnuStatus;
using FecStatus = Cru::FecStatus;

Ttc::Ttc(std::shared_ptr<BarInterface> bar, int serial, int endpoint) : mBar(bar), mSerial(serial), mEndpoint(endpoint)
{
}

void Ttc::setClock(uint32_t clock)
{
  configurePlls(clock);

  mBar->writeRegister(Cru::Registers::LOCK_CLOCK_TO_REF.index, 0);
  mBar->modifyRegister(Cru::Registers::TTC_DATA.index, 0, 2, clock);
}

void Ttc::configurePlls(uint32_t clock)
{

  uint32_t chipAddress = 0x68; //fixed address

  std::vector<std::pair<uint32_t, uint32_t>> registerMap1;
  std::vector<std::pair<uint32_t, uint32_t>> registerMap2;
  std::vector<std::pair<uint32_t, uint32_t>> registerMap3 = getSi5344RegisterMap();

  if (clock == Cru::CLOCK_LOCAL) {
    registerMap1 = getLocalClockPll1RegisterMap();
    registerMap2 = getLocalClockPll2RegisterMap();
  } else {
    setRefGen();
    registerMap1 = getTtcClockPll1RegisterMap();
    registerMap2 = getTtcClockPll2RegisterMap();
  }

  extern bool testModeORC501; // testMode flag used for some FW dev, cf JIRA ORC-501
  if (testModeORC501) { // testMode flag used for some FW dev, cf JIRA ORC-501 + JIRA ORC-511
    registerMap1 = getORC511PllRegisterMap();
    registerMap2 = getORC511PllRegisterMap();
  }

  I2c p1 = I2c(Cru::Registers::SI5345_1.address, chipAddress, mBar, 0, registerMap1);
  I2c p2 = I2c(Cru::Registers::SI5345_2.address, chipAddress, mBar, 0, registerMap2);
  I2c p3 = I2c(Cru::Registers::SI5344.address, chipAddress, mBar, 0, registerMap3);

  // lock I2C operation
  mI2cLock = std::make_unique<Interprocess::Lock>("_Alice_O2_RoC_I2C_" + std::to_string(mSerial) + "_lock", true);
  p1.configurePll();
  p2.configurePll();
  p3.configurePll();
  mI2cLock.reset();

  std::this_thread::sleep_for(std::chrono::seconds(2));
}

void Ttc::setRefGen(int frequency)
{
  uint32_t refGenFrequency = 0x0;
  uint32_t address = Cru::Registers::PON_WRAPPER_REG.address + 0x48;

  if (frequency == 40) {
    refGenFrequency = 0x80000000;
  } else if (frequency == 120) {
    refGenFrequency = 0x80000001;
  } else if (frequency == 240) {
    refGenFrequency = 0x80000002;
  } else if (frequency == 0) {
    refGenFrequency = 0x80000003;
  }

  mBar->writeRegister(address / 4, 0x0);
  mBar->writeRegister(address / 4, refGenFrequency);
}

void Ttc::resetFpll()
{
  mBar->modifyRegister(Cru::Registers::CLOCK_CONTROL.index, 24, 1, 0x1);
  mBar->modifyRegister(Cru::Registers::CLOCK_CONTROL.index, 24, 1, 0x0);
}

bool Ttc::configurePonTx(uint32_t onuAddress)
{
  // Disable automatic phase scan
  mBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x1);

  // Perform phase scan manually
  int count = 0;
  bool lowSeen = false;
  uint32_t onuStatus;
  int steps = 50000;
  int i;
  for (i = 0; i < steps; i++) {
    // Toggle phase step bit
    mBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x00300000);
    mBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x00200000);
    onuStatus = mBar->readRegister((Cru::Registers::ONU_USER_LOGIC.address + 0xC) / 4);

    // Check if ONU status bits are all '1' (ONU operational bit is not necessary)
    if (onuStatus == 0xff || onuStatus == 0xf7) {
      count++;
    } else if (onuStatus == 0xf5 || onuStatus == 0xfd) {
      count = 0;
      lowSeen = true;
    } else {
      count = 0;
    }

    if (lowSeen && count == 19) {
      break;
    }
  }

  if (i == steps) {
    return false;
  }

  // Assign ONU address
  mBar->modifyRegister(Cru::Registers::ONU_USER_LOGIC.index, 1, 8, onuAddress);

  return true;
  //TODO: Show calibration status..
}

// The monitoring parameter is set when roc-status is called through the monitoring infrastructure
// this reports and clears a different register, so that clock information is not lost
// from rogue invocations of roc-status
OnuStickyStatus Ttc::getOnuStickyStatus(bool monitoring)
{
  LinkStatus upstreamStatus = LinkStatus::Down, downstreamStatus = LinkStatus::Down;
  uint32_t was, is;
  if (monitoring) {
    was = mBar->readRegister(Cru::Registers::TTC_ONU_STICKY_MON.index);
    mBar->writeRegister(Cru::Registers::TTC_ONU_STICKY_MON.index, 0x1);
    mBar->writeRegister(Cru::Registers::TTC_ONU_STICKY_MON.index, 0x0);
    is = mBar->readRegister(Cru::Registers::TTC_ONU_STICKY_MON.index);
  } else {
    was = mBar->readRegister(Cru::Registers::TTC_ONU_STICKY.index);
    mBar->modifyRegister(Cru::Registers::TTC_DATA.index, 28, 1, 0x1);
    mBar->modifyRegister(Cru::Registers::TTC_DATA.index, 28, 1, 0x0);
    is = mBar->readRegister(Cru::Registers::TTC_ONU_STICKY.index);
  }

  // onu sticky bits: [mgtRxPllLocked][mgtTxPllLocked][mgtRxReady][mgtTxReady][operational][rxLocked][phaseGood][rx40Locked]
  // bit 0 = no error = GOOD | bit 1 = error = BAD
  // first we invert the sticky status: after inversion 1 = GOOD
  auto notIs = ~is;

  // mgtTxPllLocked | mgtTxReady | operational | phaseGood <-- these should be GOOD for upstream to be UP
  if ((notIs & 0b01011010) == 0b01011010) {
    upstreamStatus = LinkStatus::Up;
  }

  // mgtRxPllLocked | mgtRxReady | rxLocked | rx40Locked <-- these should be GOOD for downstream to be UP
  if ((notIs & 0b10100101) == 0b10100101) {
    downstreamStatus = LinkStatus::Up;
  }

  return {
    upstreamStatus,
    downstreamStatus,
    is,
    was
  };
}

OnuStatus Ttc::onuStatus(bool monitoring)
{
  uint32_t calStatus = mBar->readRegister(Cru::Registers::ONU_USER_LOGIC.index + 0xc / 4);
  uint32_t onuAddress = mBar->readRegister(Cru::Registers::ONU_USER_LOGIC.index) >> 1;
  uint32_t onuGlitchCounter = mBar->readRegister(Cru::Registers::TTC_ONU_GLITCH_COUNTER.index);

  return {
    onuAddress,
    (calStatus & 0x1) == 1, // comparisons to bool
    (calStatus >> 1 & 0x1) == 1,
    (calStatus >> 2 & 0x1) == 1,
    (calStatus >> 3 & 0x1) == 1,
    (calStatus >> 4 & 0x1) == 1,
    (calStatus >> 5 & 0x1) == 1,
    (calStatus >> 6 & 0x1) == 1,
    (calStatus >> 7 & 0x1) == 1,
    getOnuStickyStatus(monitoring),
    getPonQuality(),
    getPonQualityStatus(),
    getPonRxPower(),
    onuGlitchCounter,
  };
}

FecStatus Ttc::fecStatus()
{
  uint32_t fecStatus = mBar->readRegister(Cru::Registers::ONU_FEC_COUNTERS_STATUS.index);
  return {
    Utilities::getBit(fecStatus, 0) == 1,
    Utilities::getBit(fecStatus, 1) == 1,
    0, // meaningless, kept for compatibility
    (uint8_t)mBar->readRegister(Cru::Registers::ONU_FEC_COUNTERS_SERR.index),
    (uint8_t)mBar->readRegister(Cru::Registers::ONU_FEC_COUNTERS_DERR.index),
    0 // meaningless, kept for compatibility
  };
}

void Ttc::calibrateTtc()
{
  // Reset ONU core
  mBar->modifyRegister(Cru::Registers::ONU_USER_LOGIC.index, 0, 1, 0x1);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  mBar->modifyRegister(Cru::Registers::ONU_USER_LOGIC.index, 0, 1, 0x0);

  // Switch to refclk #0
  uint32_t sel0 = mBar->readRegister((Cru::Registers::PON_WRAPPER_PLL.address + 0x044c) / 4);
  mBar->writeRegister((Cru::Registers::PON_WRAPPER_PLL.address + 0x0448) / 4, sel0);

  // Calibrate PON RX
  Cru::rxcal0(mBar, Cru::Registers::PON_WRAPPER_TX.address);

  // Calibrate fPLL
  Cru::fpllref0(mBar, Cru::Registers::CLOCK_ONU_FPLL.address, 1); //select refclk 1
  Cru::fpllcal0(mBar, Cru::Registers::CLOCK_ONU_FPLL.address, false);

  // Calibrate ATX PLL
  Cru::atxcal0(mBar, Cru::Registers::PON_WRAPPER_PLL.address);

  //Calibrate PON TX
  Cru::txcal0(mBar, Cru::Registers::PON_WRAPPER_TX.address);

  std::this_thread::sleep_for(std::chrono::seconds(2));

  //Check MGT RX ready, RX locked and RX40 locked
  uint32_t calStatus = mBar->readRegister((Cru::Registers::ONU_USER_LOGIC.address + 0xc) / 4);
  if (((calStatus >> 5) & (calStatus >> 2) & calStatus & 0x1) != 0x1) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("PON RX Calibration failed"));
  }
}

void Ttc::selectDownstreamData(uint32_t downstreamData)
{
  mBar->modifyRegister(Cru::Registers::TTC_DATA.index, 16, 2, downstreamData);
}

uint32_t Ttc::getDownstreamData()
{
  uint32_t downstreamData = mBar->readRegister(Cru::Registers::TTC_DATA.index);
  return (downstreamData >> 16) & 0x3;
}

uint32_t Ttc::getPllClock()
{
  uint32_t chipAddress = 0x68;
  I2c p2 = I2c(Cru::Registers::SI5345_2.address, chipAddress, mBar); // no register map for this

  // lock I2C operation
  mI2cLock = std::make_unique<Interprocess::Lock>("_Alice_O2_RoC_I2C_" + std::to_string(mSerial) + "_lock", true);
  uint32_t clock = p2.getSelectedClock();
  mI2cLock.reset();

  return clock;
}

uint32_t Ttc::getHbTriggerLtuCount()
{
  return mBar->readRegister(Cru::Registers::LTU_HBTRIG_CNT.index);
}

uint32_t Ttc::getPhyTriggerLtuCount()
{
  return mBar->readRegister(Cru::Registers::LTU_PHYSTRIG_CNT.index);
}

std::pair<uint32_t, uint32_t> Ttc::getEoxSoxLtuCount()
{
  uint32_t eoxSox = mBar->readRegister(Cru::Registers::LTU_EOX_SOX_CNT.index);
  return { (eoxSox >> 4) & 0xf, eoxSox & 0xf };
}

uint32_t Ttc::getTofTriggerLtuCount()
{
  return mBar->readRegister(Cru::Registers::LTU_TOFTRIG_CNT.index) & 0xffff;
}

uint32_t Ttc::getCalTriggerLtuCount()
{
  return mBar->readRegister(Cru::Registers::LTU_CALTRIG_CNT.index) >> 16;
}

/*** CTP EMULATOR METHODS ***/
void Ttc::resetCtpEmulator(bool doReset)
{
  if (doReset) {
    mBar->writeRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 0x3); // go idle
    mBar->modifyRegister(Cru::Registers::CTP_EMU_CTRL.index, 31, 1, 0x1);
  } else {
    mBar->modifyRegister(Cru::Registers::CTP_EMU_CTRL.index, 31, 1, 0x0);
  }
}

void Ttc::setEmulatorTriggerMode(Cru::TriggerMode mode)
{
  mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 0, 2, 0x3); // always go through idle

  if (mode == Cru::TriggerMode::Manual) {
    mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 0, 2, 0x0);
  } else if (mode == Cru::TriggerMode::Periodic) {
    mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 0, 2, 0x1);
  } else if (mode == Cru::TriggerMode::Continuous) {
    mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 0, 2, 0x2);
  }
}

void Ttc::doManualPhyTrigger()
{
  mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 8, 1, 0x1); // set bit
  mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 8, 1, 0x0); // clear bit
}

void Ttc::setEmulatorContinuousMode()
{
  mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 0, 2, 0x3); // always go through idle
  mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 0, 2, 0x2);
}

void Ttc::setEmulatorIdleMode()
{
  mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 0, 2, 0x3);
}

void Ttc::setEmulatorStandaloneFlowControl(bool allow)
{
  uint32_t value = (allow ? 0x1 : 0x0);
  mBar->modifyRegister(Cru::Registers::CTP_EMU_RUNMODE.index, 2, 1, value);
}

void Ttc::setEmulatorBCMAX(uint32_t bcmax)
{
  if (bcmax > MAX_BCID) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD BCMAX VALUE") << ErrorInfo::ConfigValue(bcmax));
  } else {
    mBar->writeRegister(Cru::Registers::CTP_EMU_BCMAX.index, bcmax);
  }
}

void Ttc::setEmulatorHBMAX(uint32_t hbmax)
{
  if (hbmax > ((0x1 << 16) - 1)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD HBMAX VALUE") << ErrorInfo::ConfigValue(hbmax));
  } else {
    mBar->writeRegister(Cru::Registers::CTP_EMU_HBMAX.index, hbmax);
  }
}

/// Specify number of Heartbeat Frames to keep and drop
/// Cycles always start with keep and alternate with HB to keep and to drop
void Ttc::setEmulatorPrescaler(uint32_t hbkeep, uint32_t hbdrop)
{
  if (hbkeep > ((0x1 << 16) - 1) || hbkeep < 2) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD HBKEEP VALUE must be >= 2 and < 0xffff")
                                      << ErrorInfo::ConfigValue(hbkeep));
  }

  if (hbdrop > ((0x1 << 16) - 1) || hbdrop < 2) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD HBDROP VALUE must be >= 2 and < 0xffff")
                                      << ErrorInfo::ConfigValue(hbdrop));
  }

  mBar->writeRegister(Cru::Registers::CTP_EMU_PRESCALER.index, (hbdrop << 16) | hbkeep);
}

/// Generate a physics trigger every PHYSDIV ticks (max 28bit), larger than 7 to activate
void Ttc::setEmulatorPHYSDIV(uint32_t physdiv)
{
  if (physdiv > ((0x1 << 28) - 1)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD PHYSDIV VALUE") << ErrorInfo::ConfigValue(physdiv));
  }

  mBar->writeRegister(Cru::Registers::CTP_EMU_PHYSDIV.index, physdiv);
}

/// Generate a calibration trigger every CALDIV ticks (max 28bit), larger than 18 to activate
void Ttc::setEmulatorCALDIV(uint32_t caldiv)
{
  if (caldiv > ((0x1 << 28) - 1)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD CALDIV VALUE") << ErrorInfo::ConfigValue(caldiv));
  }

  mBar->writeRegister(Cru::Registers::CTP_EMU_CALDIV.index, caldiv);
}

/// Generate a healthcheck trigger every HCDIV ticks (max 28bit), larger than 10 to activate
void Ttc::setEmulatorHCDIV(uint32_t hcdiv)
{
  if (hcdiv > ((0x1 << 28) - 1)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD HCDIV VALUE") << ErrorInfo::ConfigValue(hcdiv));
  }

  mBar->writeRegister(Cru::Registers::CTP_EMU_HCDIV.index, hcdiv);
}

/// Set trigger at fixed bunch crossings. Always at 9 values, a value of 0 deactivates the slot
void Ttc::setFixedBCTrigger(std::vector<uint32_t> FBCTVector)
{
  if (FBCTVector.size() != 9) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD FBCT VECTOR LENGTH") << ErrorInfo::ConfigValue(FBCTVector.size()));
  } else {
    for (auto& value : FBCTVector) {
      uint32_t newValue;

      if (value > MAX_BCID) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("INVALID FBCT VALUE"));
      } else if (value == 0) {
        newValue = 0;
      } else if (value <= 2) {
        newValue = MAX_BCID - (2 - value);
      } else {
        newValue = value - 2;
      }

      mBar->writeRegister(Cru::Registers::CTP_EMU_FBCT.index, newValue);
    }
  }
}

void Ttc::setEmulatorORBITINIT(uint32_t orbitInit)
{
  mBar->writeRegister(Cru::Registers::CTP_EMU_ORBIT_INIT.index, orbitInit);
}

uint32_t Ttc::getPonQuality()
{
  return mBar->readRegister(Cru::Registers::TTC_PON_QUALITY.index);
}

int Ttc::getPonQualityStatus()
{
  uint32_t ponQuality = getPonQuality();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  if (ponQuality == getPonQuality()) {
    return 1; // GOOD
  }

  return 0; // BAD
}

double Ttc::getPonRxPower()
{
  I2c i2c = I2c(Cru::Registers::BSP_I2C_SFP_1.address, 0x51, mBar, mEndpoint);

  // lock I2C operations
  std::unique_ptr<Interprocess::Lock> i2cLock = std::make_unique<Interprocess::Lock>("_Alice_O2_RoC_I2C_" + std::to_string(mSerial) + "_lock", true);
  auto rxPower = i2c.getRxPower();
  i2cLock.reset();

  return rxPower;
}

// Currently unused by RoC
/*void Ttc::atxref(uint32_t refClock)
{
  //Was not used... (just for info purposes)
  //uint32_t reg112 = readRegister(getAtxPllRegisterAddress(0, 0x112)/4); //get in gbt for now
  uint32_t data = readRegister(getAtxPllRegisterAddress(0, 0x113 + refClock)/4);
  mBar->writeRegister(getAtxPllRegisterAddress(0, 0x112)/4, data);
}*/

} // namespace roc
} // namespace o2
