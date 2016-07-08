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

DummyChannelMaster::DummyChannelMaster(int serial, int channel, const ChannelParameters& params) : pageCounter(128)
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
  auto handle = ChannelMasterInterface::PageHandle(pageCounter);
  pageCounter++;
  return handle;
}

bool DummyChannelMaster::isPageArrived(const PageHandle& handle)
{
  cout << "DummyChannelMaster::isPageArrived(handle:" << handle.index << ")" << endl;
  return true;
}

Page DummyChannelMaster::getPage(const PageHandle& handle)
{
  cout << "DummyChannelMaster::getPage(handle:" << handle.index << ")" << endl;

  pageBuffer[0] = handle.index;
  for (int i = 1; i < pageBuffer.size(); ++i) {
    pageBuffer[i] = i - 1;
  }

  auto page = Page(pageBuffer.data(), pageBuffer.size());
  return page;
}

void DummyChannelMaster::markPageAsRead(const PageHandle& handle)
{
  cout << "DummyChannelMaster::markPageAsRead(handle:" << handle.index << ")" << endl;
}

} // namespace Rorc
} // namespace AliceO2
