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

Gbt::Gbt(std::shared_ptr<Pda::PdaBar> pdaBar, std::vector<Link> linkList, int wrapperCount) :
  mPdaBar(pdaBar),
  mLinkList(linkList),
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
  atxcal();
  cdrref(0);
  txcal();
  rxcal();
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
  for (auto const& link: mLinkList) {
    //uint32_t reg141 = readRegister(getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x141)/4);
    uint32_t data = mPdaBar->readRegister(Cru::getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x16A + refClock)/4);
    mPdaBar->writeRegister(Cru::getXcvrRegisterAddress(link.wrapper, link.bank, link.id, 0x141)/4, data);
  }
}

void Gbt::rxcal()
{
  for (auto const& link: mLinkList) {
    Cru::rxcal0(mPdaBar, link.baseAddress);
  }
}

void Gbt::txcal()
{
  for (auto const& link: mLinkList) {
    Cru::txcal0(mPdaBar, link.baseAddress);
  }
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

} // namespace roc
} // namespace AliceO2
