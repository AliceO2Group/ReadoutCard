///
/// \file CardDummy.cxx
/// \author Pascal Boeschoten
///

#include "RORC/CardDummy.h"

#include <iostream>
#include <iomanip>

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

CardDummy::CardDummy()
{
}

CardDummy::~CardDummy()
{
}

void CardDummy::startDma(int channel)
{
  cout << "CardDummy::startDma(channel=" << channel << ")" << endl;
}

void CardDummy::stopDma(int channel)
{
  cout << "CardDummy::stopDma(channel=" << channel << ")" << endl;
}

void CardDummy::openChannel(int channel, const ChannelParameters& channelParameters)
{
  cout << "CardDummy::openChanel(channel=" << channel << ", channelParameters=...)" << endl;
}

void CardDummy::closeChannel(int channel)
{
  cout << "CardDummy::closeChannel(channel=" << channel << ")" << endl;
}

void CardDummy::resetCard(int channel, ResetLevel::type resetLevel)
{
  cout << "CardDummy::resetCard(channel=" << channel << ", resetLevel=" << resetLevel << ")" << endl;
}

uint32_t CardDummy::readRegister(int channel, int index)
{
  cout << "CardDummy::readRegister(channel=" << channel << ", index=" << index << ")" << endl;
  return 0;
}

void CardDummy::writeRegister(int channel, int index, uint32_t value)
{
  cout << "CardDummy::writeRegister(channel=" << channel << ", index=" << index << ", value=" << value << ")" << endl;
}

int CardDummy::getNumberOfChannels()
{
  cout << "CardDummy::getNumberOfChannels()" << endl;
  return 0;
}

PageVector CardDummy::getMappedPages(int channel)
{
  cout << "CardDummy::getMappedPages(channel=" << channel << ")" << endl;
  return PageVector();
}

PageHandle CardDummy::pushNextPage(int channel)
{
  cout << "CardDummy::pushNextPage(channel=" << channel << ")" << endl;
  return PageHandle(-1);
}

bool CardDummy::isPageArrived(int channel, const PageHandle& handle)
{
  cout << "CardDummy::isPageArrived(channel=" << channel << ", handle=" << handle.index << ")" << endl;
  return false;
}

Page CardDummy::getPage(int channel, const PageHandle& handle)
{
  cout << "CardDummy::getPage(channel=" << channel << ", handle=" << handle.index << ")" << endl;
  return Page();
}

void CardDummy::markPageAsRead(int channel, const PageHandle& handle)
{
  cout << "CardDummy::markPageAsRead(channel=" << channel << ", handle=" << handle.index << ")" << endl;
}

volatile void* Rorc::CardDummy::getMappedMemory(int channel)
{
  cout << "CardDummy::getMappedMemory(channel=" << channel << ")" << endl;
  return nullptr;
}

int Rorc::CardDummy::getNumberOfPages(int channel)
{
  cout << "CardDummy::getNumberOfPages(channel=" << channel << ")" << endl;
  return 0;
}

} // namespace Rorc
} // namespace AliceO2
