/// \file DummyChannelMaster.cxx
/// \brief Implementation of the DummyChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DummyChannelMaster.h"
#include <iostream>
#include <string>
#include <vector>
#include "RORC/ChannelFactory.h"

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {
namespace {
CardDescriptor makeDummyDescriptor()
{
  return {CardType::Dummy, ChannelFactory::DUMMY_SERIAL_NUMBER, PciId {"dummy", "dummy"}, PciAddress {0,0,0}};
}
}

constexpr auto endm = InfoLogger::InfoLogger::StreamOps::endm;

DummyChannelMaster::DummyChannelMaster(const Parameters& params)
    : ChannelMasterBase(makeDummyDescriptor(), params, { 0, 1, 2, 3, 4, 5, 6, 7 }), mPageCounter(128)
{
//  cout << "DummyChannelMaster::DummyChannelMaster(serial:" << serial << ", channel:" << channel << ", params:...)"
//      << endl;
  mBufferSize = params.getDmaBufferSize().get_value_or(8*1024);
  mPageSize = params.getDmaPageSize().get_value_or(8*1024);

  mMaxPages = mBufferSize / mPageSize;
  mPageBuffer.resize(mBufferSize, -1);
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

void DummyChannelMaster::resetChannel(ResetLevel::type resetLevel)
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
