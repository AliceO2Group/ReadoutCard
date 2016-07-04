///
/// \file DummyChannelMaster.cxx
/// \author Pascal Boeschoten
///

#include "DummyChannelMaster.h"
#include <iostream>

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

DummyChannelMaster::DummyChannelMaster(int serial, int channel, const ChannelParameters& params)
{
  cout << "DummyChannelMaster::DummyChannelMaster()" << endl;
}

DummyChannelMaster::~DummyChannelMaster()
{
  cout << "DummyChannelMaster::~DummyChannelMaster()" << endl;
}

void DummyChannelMaster::startDma()
{
  cout << "DummyChannelMaster::startDma()" << endl;
}

void DummyChannelMaster::stopDma()
{
  cout << "DummyChannelMaster::stopDma()" << endl;
}

void DummyChannelMaster::resetCard(ResetLevel::type resetLevel)
{
  cout << "DummyChannelMaster::resetCard()" << endl;
}

uint32_t DummyChannelMaster::readRegister(int index)
{
  cout << "DummyChannelMaster::readRegister()" << endl;
  return 0;
}

void DummyChannelMaster::writeRegister(int index, uint32_t value)
{
  cout << "DummyChannelMaster::writeRegister()" << endl;
}

ChannelMasterInterface::PageHandle DummyChannelMaster::pushNextPage()
{
  cout << "DummyChannelMaster::pushNextPage()" << endl;
  return ChannelMasterInterface::PageHandle(0);
}

bool DummyChannelMaster::isPageArrived(const PageHandle& handle)
{
  cout << "DummyChannelMaster::isPageArrived()" << endl;
  return false;
}

Page DummyChannelMaster::getPage(const PageHandle& handle)
{
  cout << "DummyChannelMaster::getPage()" << endl;
  return Page();
}

void DummyChannelMaster::markPageAsRead(const PageHandle& handle)
{
  cout << "DummyChannelMaster::markPageAsRead()" << endl;
}

} // namespace Rorc
} // namespace AliceO2
