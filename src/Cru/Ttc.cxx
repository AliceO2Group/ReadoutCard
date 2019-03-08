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

namespace AliceO2 {
namespace roc {

Ttc::Ttc(std::shared_ptr<Pda::PdaBar> pdaBar) :
  mPdaBar(pdaBar)
{
}

void Ttc::setClock(uint32_t clock, bool devkit) 
{
  if (!devkit) {
    configurePlls(clock);
  }

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
    setRefGen(0);
    setRefGen(1);
    registerMap1 = getTtcClockPll1RegisterMap();
    registerMap2 = getTtcClockPll1RegisterMap();
  }

  I2c p1 = I2c(Cru::Registers::SI5345_1.address, chipAddress, mPdaBar, registerMap1);
  I2c p2 = I2c(Cru::Registers::SI5345_2.address, chipAddress, mPdaBar, registerMap2);
  I2c p3 = I2c(Cru::Registers::SI5344.address, chipAddress, mPdaBar, registerMap3);

  p1.configurePll();
  p2.configurePll();
  p3.configurePll();

  std::this_thread::sleep_for(std::chrono::seconds(2));
}

void Ttc::setRefGen(uint32_t refGenId, int frequency)
{
  uint32_t refGenFrequency = 0x0;
  uint32_t address = Cru::Registers::ONU_USER_REFGEN.index;
  if (refGenId == 0) {
    address += Cru::Registers::REFGEN0_OFFSET.index;
  }
  else if (refGenId == 1) {
    address += Cru::Registers::REFGEN1_OFFSET.index;
  }

  if (frequency == 40) {
    refGenFrequency = 0x11000000; 
  }
  else if (frequency == 120) {
    refGenFrequency = 0x11010000;
  }
  else if (frequency == 240) {
    refGenFrequency = 0x11020000;
  }
  else if (frequency == 0) {
    refGenFrequency = 0x11030000;
  }

  mPdaBar->writeRegister(address, refGenFrequency);
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
  int minimumSteps = 22;
  uint32_t onuStatus;
  int i;
  for(i=0; i<256; i++) {
    mPdaBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x00300000);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    mPdaBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x00200000);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    onuStatus = mPdaBar->readRegister((Cru::Registers::ONU_USER_LOGIC.address + 0xC)/4);
    if (onuStatus == 0xff || onuStatus == 0x7f) {
      count++;
    } else if (onuStatus == 0xf5 || onuStatus == 0xfd) {
      count = 0;
      lowSeen = true;
    } else {
      count = 0;
    }

    if (i > minimumSteps && lowSeen && count==1) {
      break;
    }
  }

  if (i==256) {
    return false;
  }

  // Assign ONU address
  mPdaBar->modifyRegister(Cru::Registers::ONU_USER_LOGIC.index, 1, 8, onuAddress);

  return true;
  //TODO: Show calibration status..
}

void Ttc::calibrateTtc()
{
  // Switch to refclk #0
  uint32_t sel0 = mPdaBar->readRegister((Cru::Registers::PON_WRAPPER_PLL.address + 0x044c)/4);
  mPdaBar->writeRegister((Cru::Registers::PON_WRAPPER_PLL.address + 0x0448)/4, sel0);

  // Calibrate PON RX
  Cru::rxcal0(mPdaBar, Cru::Registers::PON_WRAPPER_TX.address);

  // Calibrate fPLL
  Cru::fpllref0(mPdaBar, Cru::Registers::CLOCK_ONU_FPLL.address, 1); //selecte refclk 1
  Cru::fpllcal0(mPdaBar, Cru::Registers::CLOCK_ONU_FPLL.address, false);

  // Calibrate ATX PLL
  Cru::atxcal0(mPdaBar, Cru::Registers::PON_WRAPPER_PLL.address);

  //Calibrate PON TX
  Cru::txcal0(mPdaBar, Cru::Registers::PON_WRAPPER_TX.address);

  std::this_thread::sleep_for(std::chrono::seconds(2));
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
