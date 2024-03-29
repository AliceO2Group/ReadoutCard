
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
/// \file Cru/Common.h
/// \brief Implementation of common CRU utilities
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <chrono>
#include "Common.h"
#include "Utilities/Util.h"

namespace o2
{
namespace roc
{
namespace Cru
{

uint32_t getWrapperBaseAddress(int wrapper)
{
  uint32_t address = 0xffffffff;
  if (wrapper == 0) {
    address = Cru::Registers::WRAPPER0.address;
  } else if (wrapper == 1) {
    address = Cru::Registers::WRAPPER1.address;
  }
  return address;
}

uint32_t getXcvrRegisterAddress(int wrapper, int bank, int link, int reg)
{
  return getWrapperBaseAddress(wrapper) +
         Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (bank + 1) +
         Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link + 1) +
         Cru::Registers::GBT_LINK_XCVR_OFFSET.address + (4 * reg);
}

void atxcal0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress)
{

  //std::cout << "Starting ATX PLL calibration" << std::endl;

  /*uint32_t identifier = bar->readRegister((baseAddress + 4 * 0x200)/4);

  std::cout << "Base Address: 0x" << std::hex << baseAddress << 
    " Identifier: 0x" << std::hex << identifier << std::endl;
  std::cout << "Requesting internal configuration bus user access..." << std::endl;*/

  // a. Direct write of 0x2 to address 0x00 to request access (do not use modify...)

  bar->writeRegister((baseAddress + 4 * 0x000) / 4, 0x02); //...

  // b. Validate that user has control
  waitForBit(bar, baseAddress + 4 * 0x280, 2, 0);

  // c. Enable ATX PLL alibration
  bar->modifyRegister((baseAddress + 4 * 0x100) / 4, 0, 1, 0x1);

  //d. let PreSice do the calibration
  bar->modifyRegister((baseAddress + 4 * 0x000) / 4, 0, 8, 0x1);

  //e. Wait...
  waitForBit(bar, baseAddress + 4 * 0x280, 1, 0);

  //f. Calibration complete...
}

/// Calibrate XCVR TX
void txcal0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress)
{

  //a. Request access to internal configuration bus
  bar->writeRegister((baseAddress + 4 * 0x000) / 4, 0x2);

  //b. Validate that user has control
  waitForBit(bar, baseAddress + 4 * 0x280, 2, 0);

  //c. Mask out rx_cal_busy...
  bar->modifyRegister((baseAddress + 4 * 0x281) / 4, 5, 1, 0x0);

  //d. Set the Tx calibration bit
  bar->modifyRegister((baseAddress + 4 * 0x100) / 4, 5, 1, 0x1);
  bar->modifyRegister((baseAddress + 4 * 0x100) / 4, 6, 1, 0x0);

  //e. Let PreSice do the calibration
  bar->modifyRegister((baseAddress + 4 * 0x000) / 4, 0, 8, 0x1);

  //f. Wait..
  waitForBit(bar, baseAddress + 4 * 0x281, 1, 0);

  //g. Calibration is complete

  //h. Enable rx_cal_busy
  bar->modifyRegister((baseAddress + 4 * 0x281) / 4, 5, 1, 0x1);
}

/// Calibrate XCVR RX
void rxcal0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress)
{
  //a. Request access to internal configuration bus
  bar->writeRegister((baseAddress + 4 * 0x000) / 4, 0x2);

  //b. Validate that user has control
  waitForBit(bar, baseAddress + 4 * 0x280, 2, 0);

  //c. Mask out tx_cal_busy...
  bar->modifyRegister((baseAddress + 4 * 0x281) / 4, 4, 1, 0x0);

  //d. Set the Rx calibration bit
  bar->modifyRegister((baseAddress + 4 * 0x100) / 4, 1, 1, 0x1);
  bar->modifyRegister((baseAddress + 4 * 0x100) / 4, 6, 1, 0x1);

  //e. Set the rate switch flag register for PMA Rx calibration TODO(cru-sw)

  //f. Let PreSice do the calibration
  bar->modifyRegister((baseAddress + 4 * 0x000) / 4, 0, 8, 0x1);

  //g. Wait..
  waitForBit(bar, baseAddress + 4 * 0x281, 1, 0);

  //h. Calibration is complete

  //i. Enable tx_cal_busy
  bar->modifyRegister((baseAddress + 4 * 0x281) / 4, 4, 1, 0x1);
}

void fpllref0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress, uint32_t refClock)
{
  //uint32_t current114 = mPdaBar->readRegister((baseAddress + 4 * 0x114)/4);
  //uint32_t current11C = bar->readRegister((baseAddress + 4 * 0x11C)/4);

  uint32_t new114 = bar->readRegister((baseAddress + 4 * (0x117 + refClock)) / 4);
  uint32_t new11C = bar->readRegister((baseAddress + 4 * (0x11D + refClock)) / 4);

  bar->modifyRegister((baseAddress + 4 * 0x114) / 4, 0, 8, new114);
  bar->modifyRegister((baseAddress + 4 * 0x11C) / 4, 0, 8, new11C);
}

void fpllcal0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress, bool configCompensation)
{
  /*uint32_t identifier = bar->readRegister((baseAddress + 4 * 0x200)/4);

  std::cout << "Base Address: 0x" << std::hex << baseAddress << 
    " Identifier: 0x" << std::hex << identifier << std::endl;*/

  // Set fPLL to direct feedback mode
  bar->modifyRegister((baseAddress + 4 * 0x126) / 4, 0, 1, 0x1);

  //a. Request internal configuration bus (do not use modify but modify is used...)
  bar->modifyRegister((baseAddress + 4 * 0x000) / 4, 0, 8, 0x02);

  //b. Validate that user has control
  waitForBit(bar, baseAddress + 4 * 0x280, 2, 0);

  //c. Enable fPLL calibration
  bar->modifyRegister((baseAddress + 4 * 0x100) / 4, 1, 1, 0x1);

  //d. let PreSice do the calibration
  bar->modifyRegister((baseAddress + 4 * 0x000) / 4, 0, 8, 0x1);

  //e. Wait...
  waitForBit(bar, baseAddress + 4 * 0x280, 1, 0);

  //f. Calibration is complete

  //g. Set fPLL to feedback compensation mode
  if (configCompensation) {
    bar->modifyRegister((baseAddress + 4 * 0x126) / 4, 0, 1, 0x0);
  }
}

uint32_t waitForBit(std::shared_ptr<BarInterface> bar, uint32_t address, uint32_t position, uint32_t value)
{
  auto start = std::chrono::system_clock::now();
  auto curr = start;
  auto elapsed = curr - start;

  uint32_t readValue = bar->readRegister(address / 4);
  uint32_t bit = Utilities::getBit(readValue, position);

  while ((elapsed <= std::chrono::milliseconds(500)) && bit != value) {
    readValue = bar->readRegister(address / 4);
    bit = Utilities::getBit(readValue, position);
    curr = std::chrono::system_clock::now();
    elapsed = curr - start;
  }
  return bit;
}

void fpllref(std::map<int, Link> linkMap, std::shared_ptr<BarInterface> mPdaBar, uint32_t refClock, uint32_t baseAddress) //baseAddress = 0
{
  if (baseAddress == 0) {
    int prevWrapper = -1;
    int prevBank = -1;
    for (auto const& el : linkMap) {
      auto& link = el.second;
      if ((prevWrapper != link.wrapper) || (prevBank != link.bank)) {
        Cru::fpllref0(mPdaBar, getBankPllRegisterAddress(link.wrapper, link.bank), refClock);
        prevWrapper = link.wrapper;
        prevBank = link.bank;
      }
    }
  } else
    Cru::fpllref0(mPdaBar, baseAddress, refClock);
}

void fpllcal(std::map<int, Link> linkMap, std::shared_ptr<BarInterface> mPdaBar, uint32_t baseAddress, bool configCompensation) //baseAddress = 0, configCompensation = true
{
  if (baseAddress == 0) {
    int prevWrapper = -1;
    int prevBank = -1;
    for (auto const& el : linkMap) {
      auto& link = el.second;
      if ((prevWrapper != link.wrapper) || (prevBank != link.bank)) {
        Cru::fpllcal0(mPdaBar, getBankPllRegisterAddress(link.wrapper, link.bank), configCompensation);
        prevWrapper = link.wrapper;
        prevBank = link.bank;
      }
    }
  } else
    Cru::fpllcal0(mPdaBar, baseAddress, configCompensation);
}

uint32_t getBankPllRegisterAddress(int wrapper, int bank)
{
  return Cru::getWrapperBaseAddress(wrapper) +
         Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (bank + 1) +
         Cru::Registers::GBT_BANK_FPLL.address;
}

} // namespace Cru
} // namespace roc
} // namespace o2
