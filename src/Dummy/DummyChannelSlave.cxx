///
/// \file DummyChannelSlave.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "DummyChannelSlave.h"
#include <iostream>

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

DummyChannelSlave::DummyChannelSlave(int serial, int channel)
{
  cout << "DummyChannelSlave::DummyChannelSlave(serial:" << serial << ", channel:" << channel << ")" << endl;
}

DummyChannelSlave::~DummyChannelSlave()
{
  cout << "DummyChannelSlave::~DummyChannelSlave()" << endl;
}

uint32_t DummyChannelSlave::readRegister(int index)
{
  cout << "DummyChannelSlave::readRegister(" << index << ")" << endl;
  return 0;
}

void DummyChannelSlave::writeRegister(int index, uint32_t value)
{
  cout << "DummyChannelSlave::writeRegister(index:" << index << ", value:" << value << ")" << endl;
}

CardType::type DummyChannelSlave::getCardType()
{
  return CardType::Dummy;
}

} // namespace Rorc
} // namespace AliceO2
