// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

namespace AliceO2 {
namespace roc {
namespace Cru {

uint32_t getWrapperBaseAddress(int wrapper)
{
  uint32_t address = 0xffffffff;
  if (wrapper == 0) {
    address = Cru::Registers::WRAPPER0.address;
  }
  else if (wrapper == 1) {
    address =Cru::Registers::WRAPPER1.address;
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

void atxcal0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress)
{

  //std::cout << "Starting ATX PLL calibration" << std::endl;

  /*uint32_t identifier = pdaBar->readRegister((baseAddress + 4 * 0x200)/4);

  std::cout << "Base Address: 0x" << std::hex << baseAddress << 
    " Identifier: 0x" << std::hex << identifier << std::endl;
  std::cout << "Requesting internal configuration bus user access..." << std::endl;*/

  // a. Direct write of 0x2 to address 0x00 to request access (do not use modify...)

  pdaBar->writeRegister((baseAddress + 4 * 0x000)/4, 0x02); //...

  // b. Validate that user has control
  // TODO(cru-sw): There should be a check here...
  uint32_t bit = waitForBit(pdaBar, baseAddress + 4 * 0x280, 2, 0);
  if (bit != 0) {
    std::cout << "atxcal0 0" << std::endl;
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("atxcal0: User does not have control"));
  }

  // c. Enable ATX PLL alibration
  pdaBar->modifyRegister((baseAddress + 4 * 0x100)/4, 0, 1, 0x1);

  //d. let PreSice do the calibration
  pdaBar->modifyRegister((baseAddress + 4 * 0x000)/4, 0, 8, 0x1);

  //e. Wait...
  bit = waitForBit(pdaBar, baseAddress + 4 * 0x280, 1, 0);
  if (bit != 0) {
    std::cout << "atxcal0 1" << std::endl;
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("atxcal0: User does not have control"));
  }


  //f. Calibration complete...
}

/// Calibrate XCVR TX
void txcal0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress)
{ 

  //a. Request access to internal configuration bus
  pdaBar->writeRegister((baseAddress + 4 * 0x000)/4, 0x2);

  //b. Validate that user has control
  uint32_t bit = waitForBit(pdaBar, baseAddress + 4 * 0x280, 2, 0);
  if (bit != 0) {
    std::cout << "txcal0 0" << std::endl;
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("txcal0: User does not have control"));
  }

  //c. Mask out rx_cal_busy...
  pdaBar->modifyRegister((baseAddress + 4 * 0x281)/4, 5, 1, 0x0);

  //d. Set the Tx calibration bit
  pdaBar->modifyRegister((baseAddress + 4 * 0x100)/4, 5, 1, 0x1);
  pdaBar->modifyRegister((baseAddress + 4 * 0x100)/4, 6, 1, 0x0);

  //e. Let PreSice do the calibration
  pdaBar->modifyRegister((baseAddress + 4 * 0x000)/4, 0, 8, 0x1);

  //f. Wait..
  bit = waitForBit(pdaBar, baseAddress + 4 * 0x281, 1, 0);
  if (bit != 0) {
    std::cout << "txcal0 1" << std::endl;
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("txcal0: User does not have control"));
  }

  //g. Calibration is complete
  
  //h. Enable rx_cal_busy
  pdaBar->modifyRegister((baseAddress + 4 * 0x281)/4, 5, 1, 0x1);
}

/// Calibrate XCVR RX
void rxcal0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress)
{ 
  //a. Request access to internal configuration bus
  pdaBar->writeRegister((baseAddress + 4 * 0x000)/4, 0x2);

  //b. Validate that user has control
  uint32_t bit = waitForBit(pdaBar, baseAddress + 4 * 0x280, 2, 0);
  if (bit != 0) {
    std::cout << "rxcal0 0" << std::endl;
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("rxcal0: User does not have control"));
  }

  //c. Mask out tx_cal_busy...
  pdaBar->modifyRegister((baseAddress + 4 * 0x281)/4, 4, 1, 0x0);

  //d. Set the Rx calibration bit
  pdaBar->modifyRegister((baseAddress + 4 * 0x100)/4, 1, 1, 0x1);
  pdaBar->modifyRegister((baseAddress + 4 * 0x100)/4, 6, 1, 0x1);

  //e. Set the rate switch flag register for PMA Rx calibration TODO(cru-sw)
  
  //f. Let PreSice do the calibration
  pdaBar->modifyRegister((baseAddress + 4 * 0x000)/4, 0, 8, 0x1);

  //g. Wait..
  bit = waitForBit(pdaBar, baseAddress + 4 * 0x281, 1, 0);
  if (bit != 0) {
    std::cout << "rxcal0 1" << std::endl;
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("rxcal0: User does not have control"));
  }

  //h. Calibration is complete
  
  //i. Enable tx_cal_busy
  pdaBar->modifyRegister((baseAddress + 4 * 0x281)/4, 4, 1, 0x1);
}

void fpllref0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress, uint32_t refClock)
{
  //uint32_t current114 = mPdaBar->readRegister((baseAddress + 4 * 0x114)/4);
  //uint32_t current11C = pdaBar->readRegister((baseAddress + 4 * 0x11C)/4);

  uint32_t new114 = pdaBar->readRegister((baseAddress + 4 * (0x117 + refClock))/4);
  uint32_t new11C = pdaBar->readRegister((baseAddress + 4 * (0x11D + refClock))/4);

  pdaBar->modifyRegister((baseAddress + 4 * 0x114)/4, 0, 8, new114);
  pdaBar->modifyRegister((baseAddress + 4 * 0x11C)/4, 0, 8, new11C);
}

void fpllcal0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress, bool configCompensation)
{
  /*uint32_t identifier = pdaBar->readRegister((baseAddress + 4 * 0x200)/4);

  std::cout << "Base Address: 0x" << std::hex << baseAddress << 
    " Identifier: 0x" << std::hex << identifier << std::endl;*/

  // Set fPLL to direct feedback mode
  pdaBar->modifyRegister((baseAddress + 4 * 0x126)/4, 0, 1, 0x1);

  //a. Request internal configuration bus (do not use modify but modify is used...)
  pdaBar->modifyRegister((baseAddress + 4 * 0x000)/4, 0, 8, 0x02);

  //b. Validate that user has control
  uint32_t bit = waitForBit(pdaBar, baseAddress + 4 * 0x280, 2, 0);
  if (bit != 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("fpllcal0: User does not have control"));
  }

  //c. Enable fPLL calibration
  pdaBar->modifyRegister((baseAddress + 4 * 0x100)/4, 1, 1, 0x1);

  //d. let PreSice do the calibration
  pdaBar->modifyRegister((baseAddress + 4 * 0x000)/4, 0, 8, 0x1);

  //e. Wait...
  bit = waitForBit(pdaBar, baseAddress + 4 * 0x280, 1, 0);
  if (bit != 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("fpllcal0: User does not have control"));
  }

  //f. Calibration is complete
  
  //g. Set fPLL to feedback compensation mode
  if (configCompensation) {
    pdaBar->modifyRegister((baseAddress + 4 * 0x126)/4, 0, 1, 0x0);
  }
}

uint32_t waitForBit(std::shared_ptr<Pda::PdaBar> pdaBar,uint32_t address, uint32_t position, uint32_t value)
{
  auto start = std::chrono::system_clock::now();
  auto curr = start;
  auto elapsed = curr - start;

  uint32_t readValue = pdaBar->readRegister(address/4);
  uint32_t bit = Utilities::getBit(readValue, position);

  while ((elapsed <= std::chrono::milliseconds(800)) && bit != value){
    readValue = pdaBar->readRegister(address/4);
    bit = Utilities::getBit(readValue, position);
    curr = std::chrono::system_clock::now();
    elapsed = curr - start;
  }
  return bit;
}

void fpllref(std::map<int, Link> linkMap, std::shared_ptr<Pda::PdaBar> mPdaBar, uint32_t refClock, uint32_t baseAddress) //baseAddress = 0
{
  if (baseAddress == 0){
    int prevWrapper = -1;
    int prevBank = -1;
    for (auto const& el: linkMap) {
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

void fpllcal(std::map<int, Link> linkMap, std::shared_ptr<Pda::PdaBar> mPdaBar, uint32_t baseAddress, bool configCompensation) //baseAddress = 0, configCompensation = true
{
  if (baseAddress == 0){
    int prevWrapper = -1;
    int prevBank = -1;
    for (auto const& el: linkMap) {
      auto& link = el.second;
      if ((prevWrapper != link.wrapper) || (prevBank!= link.bank)) {
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
} // namespace AliceO2
