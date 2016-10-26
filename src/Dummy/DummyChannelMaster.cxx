/// \file DummyChannelMaster.cxx
/// \brief Implementation of the DummyChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DummyChannelMaster.h"
#include <iostream>
#include <string>
#include <vector>
#include "Util.h"

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

DummyChannelMaster::DummyChannelMaster(int serial, int channel, const Parameters::Map& params) : mPageCounter(128)
{
//  cout << "DummyChannelMaster::DummyChannelMaster(serial:" << serial << ", channel:" << channel << ", params:...)"
//      << endl;

  std::vector<std::string> strings {
    params.at(Parameters::Keys::dmaBufferSize()),
    params.at(Parameters::Keys::dmaPageSize())
  };
  Util::convertAssign(strings, mBufferSize, mPageSize);

  mMaxPages = mBufferSize / mPageSize;
  mPageBuffer.resize(mBufferSize, -1);
  mPageManager.setAmountOfPages(mMaxPages);
}

DummyChannelMaster::~DummyChannelMaster()
{
//  cout << "DummyChannelMaster::~DummyChannelMaster()" << endl;
}

void DummyChannelMaster::startDma()
{
//  cout << "DummyChannelMaster::startDma()" << endl;
}

void DummyChannelMaster::stopDma()
{
//  cout << "DummyChannelMaster::stopDma()" << endl;
}

void DummyChannelMaster::resetCard(ResetLevel::type resetLevel)
{
//  cout << "DummyChannelMaster::resetCard(" << ResetLevel::toString(resetLevel) << ")" << endl;
}

uint32_t DummyChannelMaster::readRegister(int index)
{
//  cout << "DummyChannelMaster::readRegister(" << index << ")" << endl;
  return 0;
}

void DummyChannelMaster::writeRegister(int index, uint32_t value)
{
//  cout << "DummyChannelMaster::writeRegister(index:" << index << ", value:" << value << ")" << endl;
}


int DummyChannelMaster::fillFifo(int maxFill)
{
//  cout << "DummyChannelMaster::fillFifo()" << endl;
  auto isArrived = [&](int) { return true; };
  auto resetDescriptor = [&](int) {};
  auto push = [&](int, int) { };

  mPageManager.handleArrivals(isArrived, resetDescriptor);
  int pushCount = mPageManager.pushPages(maxFill, push);

  mPageCounter += pushCount;
  return pushCount;
}

int DummyChannelMaster::getAvailableCount()
{
  return mPageManager.getArrivedCount();
}

auto DummyChannelMaster::popPageInternal(const MasterSharedPtr& channel) -> std::shared_ptr<Page>
{
  if (auto page = mPageManager.useArrivedPage()) {
    int bufferIndex = *page;
    return std::make_shared<Page>(&mPageBuffer[bufferIndex * mPageSize], mPageSize, bufferIndex, channel);
  }
  return nullptr;
}

void DummyChannelMaster::freePage(const Page& page)
{
//  cout << "DummyChannelMaster::freePage()" << endl;
  mPageManager.freePage(page.getId());
}

CardType::type DummyChannelMaster::getCardType()
{
  return CardType::Dummy;
}

std::vector<uint32_t> DummyChannelMaster::utilityCopyFifo()
{
  return std::vector<uint32_t>();
}

void DummyChannelMaster::utilityPrintFifo(std::ostream&)
{
//  cout << "DummyChannelMaster::utilityPrintFifo()" << endl;
}

void DummyChannelMaster::utilitySetLedState(bool state)
{
//  cout << "DummyChannelMaster::utilitySetLedState(" << (state ? "ON" : "OFF") << ")" << endl;
}

void DummyChannelMaster::utilitySanityCheck(std::ostream&)
{
//  cout << "DummyChannelMaster::utilitySanityCheck()" << endl;
}

void DummyChannelMaster::utilityCleanupState()
{
}

int DummyChannelMaster::utilityGetFirmwareVersion()
{
  return 0;
}

} // namespace Rorc
} // namespace AliceO2
