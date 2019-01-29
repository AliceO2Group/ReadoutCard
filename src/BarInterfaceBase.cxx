/// \file BarInterfaceBase.cxx
/// \brief Implementation of the BarInterfaceBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

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

BarInterfaceBase::BarInterfaceBase(std::shared_ptr<Pda::PdaBar> bar)
{
  mPdaBar = std::move(bar);
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

void BarInterfaceBase::modifyRegister(int index, int position, int width, uint32_t value)
{
  mPdaBar->modifyRegister(index, position, width, value);
}

void BarInterfaceBase::log(std::string logMessage, InfoLogger::InfoLogger::Severity logLevel)
{
  mLogger << logLevel;
  mLogger << "[PCI ID: " << mRocPciDevice->getPciAddress().toString() << " | bar" << getIndex() << "] : " <<
    logMessage << InfoLogger::InfoLogger::endm;
}

} // namespace roc
} // namespace AliceO2
