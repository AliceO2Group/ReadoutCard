// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Gbt.cxx
/// \brief Implementation of the Gbt class.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Common.h"
#include "Gbt.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {

using Link = Cru::Link;

Gbt::Gbt(std::shared_ptr<Pda::PdaBar> pdaBar, std::map<int, Link> &linkMap, int wrapperCount) :
  mPdaBar(pdaBar),
  mLinkMap(linkMap),
  mWrapperCount(wrapperCount)
{
}

void Gbt::setMux(Link link, uint32_t mux)
{
  uint32_t address = Cru::Registers::GBT_MUX_SELECT.address + (link.bank * 4);
  mPdaBar->modifyRegister(address/4, link.id*4, 2, mux);
}

void Gbt::setInternalDataGenerator(Link link, uint32_t value)
{
  uint32_t address = getSourceSelectAddress(link);

  uint32_t modifiedRegister = mPdaBar->readRegister(address/4);
  Utilities::setBits(modifiedRegister, 1, 1, value);
  Utilities::setBits(modifiedRegister, 2, 1, value);
  mPdaBar->writeRegister(address/4, modifiedRegister);

  /*mPdaBar->modifyRegister(address/4, 1, 1, value);
  mPdaBar->modifyRegister(address/4, 2, 1, value);*/
}

void Gbt::setTxMode(Link link, uint32_t mode)
{
  uint32_t address = getTxControlAddress(link);
  mPdaBar->modifyRegister(address/4, 8, 1, mode);
}

void Gbt::setRxMode(Link link, uint32_t mode)
{
   uint32_t address = getRxControlAddress(link);
   mPdaBar->modifyRegister(address/4, 8, 1, mode);
}

void Gbt::setLoopback(Link link, uint32_t enabled) 
{
  uint32_t address = getSourceSelectAddress(link);
  mPdaBar->modifyRegister(address/4, 4, 1, enabled);

}

void Gbt::calibrateGbt()
{
  Cru::fpllref(mLinkMap, mPdaBar, 2);
  Cru::fpllcal(mLinkMap, mPdaBar);
  cdrref(2);
  txcal();
  rxcal();
}

void Gbt::getGbtModes()
{
  for (auto& el : mLinkMap) {
    auto& link = el.second;
    uint32_t rxControl = mPdaBar->readRegister(getRxControlAddress(link)/4);
    if (((rxControl >> 8) & 0x1) == Cru::GBT_MODE_WB) { //TODO: Change with Constants
      link.gbtRxMode = GbtMode::type::Wb;
    } else {
      link.gbtRxMode = GbtMode::type::Gbt;
    }

    uint32_t txControl = mPdaBar->readRegister(getTxControlAddress(link)/4);
    if (((txControl >> 8) & 0x1) == Cru::GBT_MODE_WB) { //TODO: Change with Constants
      link.gbtTxMode = GbtMode::type::Wb;
    } else {
      link.gbtTxMode = GbtMode::type::Gbt;
    }
  }
}

void Gbt::getGbtMuxes()
{
  for (auto& el: mLinkMap) {
    auto& link = el.second;
    uint32_t txMux = mPdaBar->readRegister((Cru::Registers::GBT_MUX_SELECT.address + link.bank * 4)/4);
    txMux = (txMux >> (link.id*4)) & 0x3;
    if (txMux == Cru::GBT_MUX_TTC) {
      link.gbtMux = GbtMux::type::Ttc;
    } else if (txMux == Cru::GBT_MUX_DDG) {
      link.gbtMux = GbtMux::type::Ddg;
    } else if (txMux == Cru::GBT_MUX_SC) {
      link.gbtMux = GbtMux::type::Sc;
    }
  }
}

void Gbt::getLoopbacks()
{
  for (auto& el: mLinkMap) {
    auto& link = el.second;
    uint32_t loopback = mPdaBar->readRegister(getSourceSelectAddress(link)/4);
    if (((loopback >> 4) & 0x1) == 0x1) {
      link.loopback = true;
    } else {
      link.loopback = false;
    }
  }
}

void Gbt::atxcal(uint32_t baseAddress)
{
  if (baseAddress == 0) {
    for (int wrapper=0; wrapper<mWrapperCount; wrapper++) {
      Cru::atxcal0(mPdaBar, getAtxPllRegisterAddress(wrapper, 0x000));
    }
  }
  else {
    Cru::atxcal0(mPdaBar, baseAddress);
  }
}

void Gbt::cdrref(uint32_t refClock)
{
  for (auto const& el: mLinkMap) {
    auto& link = el.second;
    //uint32_t reg141 = readRegister(getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x141)/4);
    uint32_t data = mPdaBar->readRegister(Cru::getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x16A + refClock)/4);
    mPdaBar->writeRegister(Cru::getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x141)/4, data);
  }
}

void Gbt::rxcal()
{
  for (auto const& el: mLinkMap) {
    auto& link = el.second;
    Cru::rxcal0(mPdaBar, link.baseAddress);
  }
}

void Gbt::txcal()
{
  for (auto const& el: mLinkMap) {
    auto& link = el.second;
    Cru::txcal0(mPdaBar, link.baseAddress);
  }
}

uint32_t Gbt::getStatusAddress(Link link)
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
    Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
    Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
    Cru::Registers::GBT_LINK_REGS_OFFSET.address +
    Cru::Registers::GBT_LINK_STATUS.address;

  return address;
}

uint32_t Gbt::getClearErrorAddress(Link link)
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
    Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
    Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
    Cru::Registers::GBT_LINK_REGS_OFFSET.address +
    Cru::Registers::GBT_LINK_CLEAR_ERRORS.address;

  return address;
}

uint32_t Gbt::getSourceSelectAddress(Link link)
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
    Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
    Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
    Cru::Registers::GBT_LINK_REGS_OFFSET.address +
    Cru::Registers::GBT_LINK_SOURCE_SELECT.address;

  return address;
}

uint32_t Gbt::getTxControlAddress(Link link)
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
    Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
    Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
    Cru::Registers::GBT_LINK_REGS_OFFSET.address +
    Cru::Registers::GBT_LINK_TX_CONTROL_OFFSET.address;

  return address;
}

uint32_t Gbt::getRxControlAddress(Link link)
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
    Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
    Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
    Cru::Registers::GBT_LINK_REGS_OFFSET.address +
    Cru::Registers::GBT_LINK_RX_CONTROL_OFFSET.address;

  return address;
}

uint32_t Gbt::getAtxPllRegisterAddress(int wrapper, uint32_t reg)
{
  return Cru::getWrapperBaseAddress(wrapper) + 
    Cru::Registers::GBT_WRAPPER_ATX_PLL.address + 4 * reg;
}

bool Gbt::getStickyBit(Link link)
{
  uint32_t addr = getStatusAddress(link);
  uint32_t data = mPdaBar->readRegister(addr/4);
  uint32_t lockedData = Utilities::getBit(~data, 14); //phy up 1 = locked, 0 = down
  uint32_t ready = Utilities::getBit(~data, 15); //data layer up 1 = locked, 0 = down 
  if ((lockedData == 0x0) || (ready == 0x0)) {
    resetStickyBit(link);
  }

  return (lockedData == 0x1 && ready == 0x1) ? true : false;
}

void Gbt::resetStickyBit(Link link) {
  uint32_t addr = getClearErrorAddress(link);

  mPdaBar->writeRegister(addr/4, 0x0);
}

uint32_t Gbt::getRxClockFrequency(Link link) { //In Hz
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
    Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
    Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
    Cru::Registers::GBT_LINK_REGS_OFFSET.address +
    Cru::Registers::GBT_LINK_RX_CLOCK.address;

  return mPdaBar->readRegister(address/4);
}

uint32_t Gbt::getTxClockFrequency(Link link) { //In Hz
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
    Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
    Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
    Cru::Registers::GBT_LINK_REGS_OFFSET.address +
    Cru::Registers::GBT_LINK_TX_CLOCK.address;

  return mPdaBar->readRegister(address/4);
}

} // namespace roc
} // namespace AliceO2
