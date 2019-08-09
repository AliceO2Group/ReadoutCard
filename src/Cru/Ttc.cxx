// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Ttc.cxx
/// \brief Implementation of the Ttc class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <thread>
#include "Common.h"
#include "Constants.h"
#include "I2c.h"
#include "Ttc.h"
#include "register_maps/Si5345-RevD_local_pll1_zdb-Registers.h"
#include "register_maps/Si5345-RevD_local_pll2_zdb-Registers.h"
#include "register_maps/Si5345-RevD_ttc_pll1_zdb-Registers.h"
#include "register_maps/Si5345-RevD_ttc_pll2_zdb-Registers.h"
#include "register_maps/Si5344-RevD-TFC_40-Registers.h"

namespace AliceO2
{
namespace roc
{

Ttc::Ttc(std::shared_ptr<Pda::PdaBar> pdaBar) : mPdaBar(pdaBar)
{
}

void Ttc::setClock(uint32_t clock)
{
  configurePlls(clock);

  mPdaBar->writeRegister(Cru::Registers::LOCK_CLOCK_TO_REF.index, 0);
  mPdaBar->modifyRegister(Cru::Registers::TTC_DATA.index, 0, 2, clock);
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

  I2c p1 = I2c(Cru::Registers::SI5345_1.address, chipAddress, mPdaBar, registerMap1);
  I2c p2 = I2c(Cru::Registers::SI5345_2.address, chipAddress, mPdaBar, registerMap2);
  I2c p3 = I2c(Cru::Registers::SI5344.address, chipAddress, mPdaBar, registerMap3);

  p1.configurePll();
  p2.configurePll();
  p3.configurePll();

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

  mPdaBar->writeRegister(address / 4, 0x0);
  mPdaBar->writeRegister(address / 4, refGenFrequency);
}

void Ttc::resetFpll()
{
  mPdaBar->modifyRegister(Cru::Registers::CLOCK_CONTROL.index, 24, 1, 0x1);
  mPdaBar->modifyRegister(Cru::Registers::CLOCK_CONTROL.index, 24, 1, 0x0);
}

bool Ttc::configurePonTx(uint32_t onuAddress)
{
  // Disable automatic phase scan
  mPdaBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x1);

  // Perform phase scan manually
  int count = 0;
  bool lowSeen = false;
  uint32_t onuStatus;
  int steps = 50000;
  int i;
  for (i = 0; i < steps; i++) {
    // Toggle phase step bit
    mPdaBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x00300000);
    mPdaBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x00200000);
    onuStatus = mPdaBar->readRegister((Cru::Registers::ONU_USER_LOGIC.address + 0xC) / 4);

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
  mPdaBar->modifyRegister(Cru::Registers::ONU_USER_LOGIC.index, 1, 8, onuAddress);

  return true;
  //TODO: Show calibration status..
}

void Ttc::calibrateTtc()
{
  // Reset ONU core
  mPdaBar->modifyRegister(Cru::Registers::ONU_USER_LOGIC.index, 0, 1, 0x1);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  mPdaBar->modifyRegister(Cru::Registers::ONU_USER_LOGIC.index, 0, 1, 0x0);

  // Switch to refclk #0
  uint32_t sel0 = mPdaBar->readRegister((Cru::Registers::PON_WRAPPER_PLL.address + 0x044c) / 4);
  mPdaBar->writeRegister((Cru::Registers::PON_WRAPPER_PLL.address + 0x0448) / 4, sel0);

  // Calibrate PON RX
  Cru::rxcal0(mPdaBar, Cru::Registers::PON_WRAPPER_TX.address);

  // Calibrate fPLL
  Cru::fpllref0(mPdaBar, Cru::Registers::CLOCK_ONU_FPLL.address, 1); //select refclk 1
  Cru::fpllcal0(mPdaBar, Cru::Registers::CLOCK_ONU_FPLL.address, false);

  // Calibrate ATX PLL
  Cru::atxcal0(mPdaBar, Cru::Registers::PON_WRAPPER_PLL.address);

  //Calibrate PON TX
  Cru::txcal0(mPdaBar, Cru::Registers::PON_WRAPPER_TX.address);

  std::this_thread::sleep_for(std::chrono::seconds(2));

  //Check MGT RX ready, RX locked and RX40 locked
  uint32_t calStatus = mPdaBar->readRegister((Cru::Registers::ONU_USER_LOGIC.address + 0xc) / 4);
  if (((calStatus >> 5) & (calStatus >> 2) & calStatus & 0x1) != 0x1) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("PON RX Calibration failed"));
  }
}

void Ttc::selectDownstreamData(uint32_t downstreamData)
{
  mPdaBar->modifyRegister(Cru::Registers::TTC_DATA.index, 16, 2, downstreamData);
}

uint32_t Ttc::getDownstreamData()
{
  uint32_t downstreamData = mPdaBar->readRegister(Cru::Registers::TTC_DATA.index);
  return (downstreamData >> 16) & 0x3;
}

uint32_t Ttc::getPllClock()
{
  uint32_t chipAddress = 0x68;
  I2c p2 = I2c(Cru::Registers::SI5345_2.address, chipAddress, mPdaBar); // no register map for this
  uint32_t clock = p2.getSelectedClock();
  return clock;
}

// Currently unused by RoC
/*void Ttc::atxref(uint32_t refClock)
{
  //Was not used... (just for info purposes)
  //uint32_t reg112 = readRegister(getAtxPllRegisterAddress(0, 0x112)/4); //get in gbt for now
  uint32_t data = readRegister(getAtxPllRegisterAddress(0, 0x113 + refClock)/4);
  mPdaBar->writeRegister(getAtxPllRegisterAddress(0, 0x112)/4, data);
}*/

} // namespace roc
} // namespace AliceO2
