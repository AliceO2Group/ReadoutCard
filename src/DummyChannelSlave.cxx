///
/// \file DummyChannelSlave.cxx
/// \author Pascal Boeschoten
///

#include "DummyChannelSlave.h"
#include <iostream>

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

DummyChannelSlave::DummyChannelSlave(int serial, int channel)
{
  cout << "DummyChannelSlave::DummyChannelSlave()" << endl;
}

DummyChannelSlave::~DummyChannelSlave()
{
  cout << "DummyChannelSlave::~DummyChannelSlave()" << endl;
}

uint32_t DummyChannelSlave::readRegister(int index)
{
  cout << "DummyChannelSlave::readRegister()" << endl;
  return 0;
}

void DummyChannelSlave::writeRegister(int index, uint32_t value)
{
  cout << "DummyChannelSlave::writeRegister()" << endl;
}

} // namespace Rorc
} // namespace AliceO2
