/// \file BarInterfaceBase.cxx
/// \brief Implementation of the BarInterfaceBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "BarInterfaceBase.h"
#include "Utilities/SmartPointer.h"

namespace AliceO2 {
namespace roc {

BarInterfaceBase::BarInterfaceBase(const Parameters& parameters)
 : mBarIndex(parameters.getChannelNumberRequired())
{
  auto id = parameters.getCardIdRequired();
  if (auto serial = boost::get<int>(&id)) {
    Utilities::resetSmartPtr(mRocPciDevice, *serial);
  } else if (auto address = boost::get<PciAddress>(&id)) {
    Utilities::resetSmartPtr(mRocPciDevice, *address);
  }
  Utilities::resetSmartPtr(mPdaBar, mRocPciDevice->getPciDevice(), mBarIndex);
}

BarInterfaceBase::~BarInterfaceBase()
{
}

uint32_t BarInterfaceBase::readRegister(int index)
{
  // TODO Access restriction
  return mPdaBar->readRegister(index);
}

void BarInterfaceBase::writeRegister(int index, uint32_t value)
{
  // TODO Access restriction
  mPdaBar->writeRegister(index, value);
}

int BarInterfaceBase::getBarIndex() const
{
  return mBarIndex;
}


} // namespace roc
} // namespace AliceO2
