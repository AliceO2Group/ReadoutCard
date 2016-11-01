/// \file ChannelSlave.cxx
/// \brief Implementation of the ChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelSlave.h"
#include <iostream>

namespace AliceO2 {
namespace Rorc {

ChannelSlave::ChannelSlave(const Parameters& parameters)
 : mSerialNumber(parameters.getRequired<Parameters::SerialNumber>()),
   mChannelNumber(parameters.getRequired<Parameters::ChannelNumber>()),
   mRorcDevice(mSerialNumber),
   mPdaBar(mRorcDevice.getPciDevice(), mChannelNumber)
{
}

ChannelSlave::~ChannelSlave()
{
}

uint32_t ChannelSlave::readRegister(int index)
{
  // TODO Range check
  // TODO Access restriction
  return mPdaBar.getRegister<uint32_t>(index);
}

void ChannelSlave::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  // TODO Access restriction
  mPdaBar.setRegister<uint32_t>(index, value);

}

} // namespace Rorc
} // namespace AliceO2
