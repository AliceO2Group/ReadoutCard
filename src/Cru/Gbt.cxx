
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
/// \file Gbt.cxx
/// \brief Implementation of the Gbt class.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>

#include <chrono>
#include <thread>

#include "Gbt.h"
#include "Utilities/Util.h"

namespace o2
{
namespace roc
{

using Link = Cru::Link;
using LinkStatus = Cru::LinkStatus;
using LoopbackStats = Cru::LoopbackStats;

Gbt::Gbt(std::shared_ptr<Pda::PdaBar> pdaBar, std::map<int, Link>& linkMap, int wrapperCount, int endpoint) : mPdaBar(pdaBar),
                                                                                                              mLinkMap(linkMap),
                                                                                                              mWrapperCount(wrapperCount),
                                                                                                              mEndpoint(endpoint)
{
}

void Gbt::setMux(int index, uint32_t mux)
{
  if (mEndpoint == 1) {
    index += 12;
  }
  uint32_t reg = index / 8;
  uint32_t bitOffset = (index % 8) * 4;
  uint32_t address = Cru::Registers::GBT_MUX_SELECT.address + (reg * 4);
  mPdaBar->modifyRegister(address / 4, bitOffset, 4, mux);
}

void Gbt::setInternalDataGenerator(Link link, uint32_t value)
{
  uint32_t address = getSourceSelectAddress(link);

  uint32_t modifiedRegister = mPdaBar->readRegister(address / 4);
  Utilities::setBits(modifiedRegister, 1, 1, value);
  Utilities::setBits(modifiedRegister, 2, 1, value);
  mPdaBar->writeRegister(address / 4, modifiedRegister);

  /*mPdaBar->modifyRegister(address/4, 1, 1, value);
  mPdaBar->modifyRegister(address/4, 2, 1, value);*/
}

void Gbt::setTxMode(Link link, uint32_t mode)
{
  uint32_t address = getTxControlAddress(link);
  mPdaBar->modifyRegister(address / 4, 8, 1, mode);
}

void Gbt::setRxMode(Link link, uint32_t mode)
{
  uint32_t address = getRxControlAddress(link);
  mPdaBar->modifyRegister(address / 4, 8, 1, mode);
}

void Gbt::setLoopback(Link link, uint32_t enabled)
{
  uint32_t address = getSourceSelectAddress(link);
  mPdaBar->modifyRegister(address / 4, 4, 1, enabled);
}

void Gbt::calibrateGbt(std::map<int, Link> linkMap)
{
  //Cru::fpllref(linkMap, mPdaBar, 2); //Has been bound with clock configuration
  //Cru::fpllcal(linkMap, mPdaBar); //same
  cdrref(linkMap, 2);
  txcal(linkMap);
  rxcal(linkMap);
}

void Gbt::getGbtModes()
{
  for (auto& el : mLinkMap) {
    auto& link = el.second;
    uint32_t rxControl = mPdaBar->readRegister(getRxControlAddress(link) / 4);
    if (((rxControl >> 8) & 0x1) == Cru::GBT_MODE_WB) {
      link.gbtRxMode = GbtMode::type::Wb;
    } else {
      link.gbtRxMode = GbtMode::type::Gbt;
    }

    uint32_t txControl = mPdaBar->readRegister(getTxControlAddress(link) / 4);
    if (((txControl >> 8) & 0x1) == Cru::GBT_MODE_WB) {
      link.gbtTxMode = GbtMode::type::Wb;
    } else {
      link.gbtTxMode = GbtMode::type::Gbt;
    }
  }
}

void Gbt::getGbtMuxes()
{
  for (auto& el : mLinkMap) {
    int index = el.first;
    if (mEndpoint == 1) {
      index += 12;
    }
    auto& link = el.second;
    uint32_t reg = (index / 8);
    uint32_t bitOffset = (index % 8) * 4;
    uint32_t txMux = mPdaBar->readRegister((Cru::Registers::GBT_MUX_SELECT.address + reg * 4) / 4);
    txMux = (txMux >> bitOffset) & 0xf;
    if (txMux == Cru::GBT_MUX_TTC) {
      link.gbtMux = GbtMux::type::Ttc;
    } else if (txMux == Cru::GBT_MUX_DDG) {
      link.gbtMux = GbtMux::type::Ddg;
    } else if (txMux == Cru::GBT_MUX_SWT) {
      link.gbtMux = GbtMux::type::Swt;
    } else if (txMux == Cru::GBT_MUX_TTCUP) {
      link.gbtMux = GbtMux::type::TtcUp;
    } else if (txMux == Cru::GBT_MUX_UL) {
      link.gbtMux = GbtMux::type::Ul;
    } else {
      link.gbtMux = GbtMux::type::Na;
    }
  }
}

void Gbt::getLoopbacks()
{
  for (auto& el : mLinkMap) {
    auto& link = el.second;
    uint32_t loopback = mPdaBar->readRegister(getSourceSelectAddress(link) / 4);
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
    for (int wrapper = 0; wrapper < mWrapperCount; wrapper++) {
      Cru::atxcal0(mPdaBar, getAtxPllRegisterAddress(wrapper, 0x000));
    }
  } else {
    Cru::atxcal0(mPdaBar, baseAddress);
  }
}

void Gbt::cdrref(std::map<int, Link> linkMap, uint32_t refClock)
{
  for (auto const& el : linkMap) {
    auto& link = el.second;
    //uint32_t reg141 = readRegister(getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x141)/4);
    uint32_t data = mPdaBar->readRegister(Cru::getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x16A + refClock) / 4);
    mPdaBar->writeRegister(Cru::getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x141) / 4, data);
  }
}

void Gbt::rxcal(std::map<int, Link> linkMap)
{
  for (auto const& el : linkMap) {
    auto& link = el.second;
    Cru::rxcal0(mPdaBar, link.baseAddress);
  }
}

void Gbt::txcal(std::map<int, Link> linkMap)
{
  for (auto const& el : linkMap) {
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

uint32_t Gbt::getRxErrorCount(Link link)
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
                     Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
                     Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
                     Cru::Registers::GBT_LINK_REGS_OFFSET.address +
                     Cru::Registers::GBT_LINK_RX_ERROR_COUNT.address;
  return mPdaBar->readRegister(address / 4);
}

uint32_t Gbt::getFecErrorCount(Link link)
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
                     Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
                     Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
                     Cru::Registers::GBT_LINK_REGS_OFFSET.address +
                     Cru::Registers::GBT_LINK_FEC_MONITORING.address;
  return mPdaBar->readRegister(address / 4);
}

LinkStatus Gbt::getStickyBit(Link link)
{
  uint32_t addr = getStatusAddress(link);
  uint32_t data = mPdaBar->readRegister(addr / 4);
  uint32_t lockedData = Utilities::getBit(~data, 14); //phy up 1 = locked, 0 = down
  uint32_t ready = Utilities::getBit(~data, 15);      //data layer up 1 = locked, 0 = down
  if ((lockedData == 0x0) || (ready == 0x0)) {
    resetStickyBit(link);
    data = mPdaBar->readRegister(addr / 4);
    lockedData = Utilities::getBit(~data, 14); //phy up 1 = locked, 0 = down
    ready = Utilities::getBit(~data, 15);      //data layer up 1 = locked, 0 = down

    return (lockedData == 0x1 && ready == 0x1) ? LinkStatus::UpWasDown : LinkStatus::Down;
  }

  return (lockedData == 0x1 && ready == 0x1) ? LinkStatus::Up : LinkStatus::Down;
}

void Gbt::resetStickyBit(Link link)
{
  uint32_t addr = getClearErrorAddress(link);

  mPdaBar->writeRegister(addr / 4, 0x0);
}

uint32_t Gbt::getRxClockFrequency(Link link) //In Hz
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
                     Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
                     Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
                     Cru::Registers::GBT_LINK_REGS_OFFSET.address +
                     Cru::Registers::GBT_LINK_RX_CLOCK.address;

  return mPdaBar->readRegister(address / 4);
}

uint32_t Gbt::getTxClockFrequency(Link link) //In Hz
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
                     Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
                     Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
                     Cru::Registers::GBT_LINK_REGS_OFFSET.address +
                     Cru::Registers::GBT_LINK_TX_CLOCK.address;

  return mPdaBar->readRegister(address / 4);
}

uint32_t Gbt::getGlitchCounter(Link link)
{
  uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
                     Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
                     Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
                     Cru::Registers::GBT_LINK_REGS_OFFSET.address +
                     Cru::Registers::GBT_LINK_GLITCH_COUNTER.address;

  return mPdaBar->readRegister(address / 4);
}

void Gbt::resetFifo()
{
  mPdaBar->modifyRegister(Cru::Registers::BSP_USER_CONTROL.index, 7, 1, 0x1); // reset TX
  mPdaBar->modifyRegister(Cru::Registers::BSP_USER_CONTROL.index, 8, 1, 0x1); // reset RX

  mPdaBar->modifyRegister(Cru::Registers::BSP_USER_CONTROL.index, 7, 1, 0x0);
  mPdaBar->modifyRegister(Cru::Registers::BSP_USER_CONTROL.index, 8, 1, 0x0);
}

std::map<int, LoopbackStats> Gbt::getLoopbackStats(bool reset, GbtPatternMode::type patternMode, GbtCounterType::type counterType, GbtStatsMode::type statsMode,
                                                   uint32_t lowMask, uint32_t medMask, uint32_t highMask)
{
  if (reset) {
    for (const auto& el : mLinkMap) {
      auto link = el.second;
      setInternalDataGenerator(link, 1);
    }

    setPatternMode(patternMode);
    setTxCounterType(counterType);
    setRxPatternMask(lowMask, medMask, highMask);

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); //It's not clear why this is necessary

    resetErrorCounters();
  }

  auto ret = getStats(statsMode);

  /*for (const auto& el : mLinkMap) {
    auto link = el.second;
    setInternalDataGenerator(link, 0);
  }*/
  return ret;
}

std::map<int, LoopbackStats> Gbt::getStats(GbtStatsMode::type /*statsMode*/)
{
  std::map<int, LoopbackStats> ret;
  for (const auto& el : mLinkMap) {
    const auto& link = el.second;
    uint32_t data = mPdaBar->readRegister(getStatusAddress(link) / 4);

    uint32_t pllLock = Utilities::getBit(data, 8);
    uint32_t rxLockedToData = Utilities::getBit(data, 10);
    uint32_t dataLayerUp = Utilities::getBit(data, 11);
    uint32_t gbtPhyUp = Utilities::getBit(data, 13);

    uint32_t rxDataErrorCount = getRxErrorCount(link);
    uint32_t fecErrorCount = getFecErrorCount(link);

    LoopbackStats stat = { pllLock == 0x1,
                           rxLockedToData == 0x1,
                           dataLayerUp == 0x1,
                           gbtPhyUp == 0x1,
                           rxDataErrorCount,
                           fecErrorCount };
    ret[el.first] = stat;
  }

  return ret;
}

void Gbt::setPatternMode(GbtPatternMode::type patternMode)
{
  for (const auto& el : mLinkMap) {
    const auto& link = el.second;
    uint32_t address = getSourceSelectAddress(link);
    if (patternMode == GbtPatternMode::type::Counter) {
      mPdaBar->modifyRegister(address / 4, 5, 1, 0x0);
    } else if (patternMode == GbtPatternMode::type::Static) {
      mPdaBar->modifyRegister(address / 4, 5, 1, 0x1);
    }
  }
}

void Gbt::setTxCounterType(GbtCounterType::type counterType)
{
  for (int wrapper = 0; wrapper < mWrapperCount; wrapper++) {
    uint32_t address = Cru::getWrapperBaseAddress(wrapper) + Cru::Registers::GBT_WRAPPER_GREGS.address + Cru::Registers::GBT_WRAPPER_TEST_CTRL.address;

    if (counterType == GbtCounterType::type::ThirtyBit) {
      mPdaBar->modifyRegister(address / 4, 7, 1, 0x0);
    } else if (counterType == GbtCounterType::type::EightBit) {
      mPdaBar->modifyRegister(address / 4, 7, 1, 0x1);
    }
  }
}

void Gbt::setRxPatternMask(uint32_t lowMask, uint32_t midMask, uint32_t highMask)
{
  for (const auto& el : mLinkMap) {
    const auto& link = el.second;
    uint32_t address = Cru::getWrapperBaseAddress(link.wrapper) +
                       Cru::Registers::GBT_WRAPPER_BANK_OFFSET.address * (link.bank + 1) +
                       Cru::Registers::GBT_BANK_LINK_OFFSET.address * (link.id + 1) +
                       Cru::Registers::GBT_LINK_REGS_OFFSET.address;
    mPdaBar->writeRegister((address + Cru::Registers::GBT_LINK_MASK_LOW.address) / 4, lowMask);
    mPdaBar->writeRegister((address + Cru::Registers::GBT_LINK_MASK_MED.address) / 4, midMask);
    mPdaBar->writeRegister((address + Cru::Registers::GBT_LINK_MASK_HIGH.address) / 4, highMask);
  }
}

void Gbt::resetErrorCounters()
{
  for (const auto& el : mLinkMap) {
    const auto& link = el.second;
    uint32_t address = getSourceSelectAddress(link);
    mPdaBar->modifyRegister(address / 4, 6, 1, 0x1);
    mPdaBar->modifyRegister(address / 4, 6, 1, 0x0);
  }
}

} // namespace roc
} // namespace o2
