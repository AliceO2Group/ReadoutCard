/// \file DummyChannelSlave.cxx
/// \brief Implementation of the DummyChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DummyChannelSlave.h"
#include <iostream>

using std::cout;

namespace AliceO2 {
namespace Rorc {

DummyChannelSlave::DummyChannelSlave(const Parameters& parameters)
{
  auto id = parameters.getRequired<Parameters::CardId>();
  auto channel = parameters.getRequired<Parameters::ChannelNumber>();

  cout << "DummyChannelSlave::DummyChannelSlave(";
  if (auto serial = boost::get<int>(&id)) {
    cout << "serial:" << *serial << ", ";
  }
  if (auto address = boost::get<PciAddress>(&id)) {
    cout << "address:" << address->toString() << ", ";
  }

  cout << "channel:" << channel << ")\n";
}

DummyChannelSlave::~DummyChannelSlave()
{
  cout << "DummyChannelSlave::~DummyChannelSlave()\n";
}

uint32_t DummyChannelSlave::readRegister(int index)
{
  cout << "DummyChannelSlave::readRegister(" << index << ")\n";
  return 0;
}

void DummyChannelSlave::writeRegister(int index, uint32_t value)
{
  cout << "DummyChannelSlave::writeRegister(index:" << index << ", value:" << value << ")\n";
}

CardType::type DummyChannelSlave::getCardType()
{
  return CardType::Dummy;
}

} // namespace Rorc
} // namespace AliceO2
