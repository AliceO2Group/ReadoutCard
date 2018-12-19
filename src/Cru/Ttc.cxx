/// \file Ttc.cxx
/// \brief Implementation of the Ttc class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <thread>
#include "Common.h"
#include "Constants.h"
#include "I2c.h"
#include "Ttc.h"
#include "register_maps/Regmap_default_PLL2_40mhz_input_240mhz_output.h"
#include "register_maps/Regmap_PLL2_240mhz_input_240mhz_output.h"
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

  std::vector<std::pair<uint32_t, uint32_t>> registerMap2;
  std::vector<std::pair<uint32_t, uint32_t>> registerMap3 = getSi5344RegisterMap();

  if (clock == Cru::CLOCK_LOCAL) {
    registerMap2 = getLocalClockRegisterMap();
  } else {
    setRefGen(1);
    registerMap2 = getTtcClockRegisterMap();
  }

  I2c p2 = I2c(Cru::Registers::SI5345_2.address, chipAddress, mPdaBar, registerMap2);
  I2c p3 = I2c(Cru::Registers::SI5344.address, chipAddress, mPdaBar, registerMap3);

  p2.configurePll();
  p3.configurePll();
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

void Ttc::configurePonTx(uint32_t onuAddress)
{

  Cru::fpllref0(mPdaBar, Cru::Registers::CLOCK_ONU_FPLL.address, 0x1);
  Cru::fpllcal0(mPdaBar, Cru::Registers::CLOCK_ONU_FPLL.address, false);

  mPdaBar->writeRegister(Cru::Registers::CLOCK_PLL_CONTROL_ONU.index, 0x1);
  mPdaBar->modifyRegister(Cru::Registers::ONU_USER_LOGIC.index, 1, 8, onuAddress);

  //TODO: Show calibration status..
}

void Ttc::calibrateTtc()
{
  // Switch to refclk #0
  uint32_t sel0 = mPdaBar->readRegister((Cru::Registers::PON_WRAPPER_PLL.address + 0x044c)/4);
  mPdaBar->writeRegister((Cru::Registers::PON_WRAPPER_PLL.address + 0x0448)/4, sel0);

  // Calibrate ATX PLL
  Cru::atxcal0(mPdaBar, Cru::Registers::PON_WRAPPER_PLL.address);

  //Calibrate PON TX/RX
  Cru::txcal0(mPdaBar, Cru::Registers::PON_WRAPPER_TX.address);
  Cru::rxcal0(mPdaBar, Cru::Registers::PON_WRAPPER_TX.address);

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
/*void Ttc::fpllref(uint32_t refClock, uint32_t baseAddress) //baseAddress = 0
{
  if (baseAddress == 0){
    int prevWrapper = -1;
    int prevBank = -1;
    for (auto const& link: mLinkList) {
      if ((prevWrapper != link.wrapper) || (prevBank != link.bank)) {
        Cru::fpllref0(mPdaBar, getBankPllRegisterAddress(link.wrapper, link.bank), refClock);
        prevWrapper = link.wrapper;
        prevBank = link.bank;
      }
    }
  } else
    Cru::fpllref0(mPdaBar, baseAddress, refClock);
}

void Ttc::fpllcal(uint32_t baseAddress, bool configCompensation) //baseAddress = 0, configCompensation = true
{
  if (baseAddress == 0){
    int prevWrapper = -1;
    int prevBank = -1;
    for (auto const& link: mLinkList) {
      if ((prevWrapper != link.wrapper) || (prevBank!= link.bank)) {
        Cru::fpllcal0(mPdaBar, getBankPllRegisterAddress(link.wrapper, link.bank), configCompensation);
        prevWrapper = link.wrapper;
        prevBank = link.bank;
      }
    }
  } else
    Cru::fpllcal0(mPdaBar, baseAddress, configCompensation);
}

uint32_t Ttc::getBankPllRegisterAddress(int wrapper, int bank)
{
  return Cru::getWrapperBaseAddress(wrapper) + 
    Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (bank + 1) +
    Cru::Registers::GBT_BANK_FPLL.address;
}

void Ttc::atxref(uint32_t refClock)
{
  //Was not used... (just for info purposes)
  //uint32_t reg112 = readRegister(getAtxPllRegisterAddress(0, 0x112)/4); //get in gbt for now
  uint32_t data = readRegister(getAtxPllRegisterAddress(0, 0x113 + refClock)/4);
  mPdaBar->writeRegister(getAtxPllRegisterAddress(0, 0x112)/4, data);
}*/

} // namespace roc
} // namespace AliceO2
