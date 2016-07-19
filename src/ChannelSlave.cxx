///
/// \file ChannelSlave.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "ChannelSlave.h"
#include <iostream>

namespace AliceO2 {
namespace Rorc {

ChannelSlave::ChannelSlave(int serial, int channel)
 : serialNumber(serial),
   channelNumber(channel),
   rorcDevice(serial),
   pdaBar(rorcDevice.getPciDevice(), channel)
{
}

ChannelSlave::~ChannelSlave()
{
}

uint32_t ChannelSlave::readRegister(int index)
{
  // TODO Range check
  // TODO Access restriction
  return pdaBar[index];
}

void ChannelSlave::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  // TODO Access restriction
  pdaBar[index] = value;
}

} // namespace Rorc
} // namespace AliceO2
