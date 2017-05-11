/// \file DummyChannelMaster.cxx
/// \brief Implementation of the DummyChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DummyChannelMaster.h"
#include "ReadoutCard/ChannelFactory.h"

namespace AliceO2 {
namespace roc {
namespace {
CardDescriptor makeDummyDescriptor()
{
  return {CardType::Dummy, ChannelFactory::DUMMY_SERIAL_NUMBER, PciId {"dummy", "dummy"}, PciAddress {0,0,0}};
}
}

constexpr auto endm = InfoLogger::InfoLogger::StreamOps::endm;

DummyChannelMaster::DummyChannelMaster(const Parameters& params)
    : ChannelMasterBase(makeDummyDescriptor(), params, { 0, 1, 2, 3, 4, 5, 6, 7 })
{
  getLogger() << "DummyChannelMaster::DummyChannelMaster(channel:" << params.getChannelNumberRequired() << ")"
      << InfoLogger::InfoLogger::endm;
}

DummyChannelMaster::~DummyChannelMaster()
{
  getLogger() << "DummyChannelMaster::~DummyChannelMaster()" << InfoLogger::InfoLogger::endm;
}

void DummyChannelMaster::startDma()
{
  getLogger() << "DummyChannelMaster::startDma()" << InfoLogger::InfoLogger::endm;
}

void DummyChannelMaster::stopDma()
{
  getLogger() << "DummyChannelMaster::stopDma()" << InfoLogger::InfoLogger::endm;
}

void DummyChannelMaster::resetChannel(ResetLevel::type resetLevel)
{
  getLogger() << "DummyChannelMaster::resetCard(" << ResetLevel::toString(resetLevel) << ")"
      << InfoLogger::InfoLogger::endm;
}

uint32_t DummyChannelMaster::readRegister(int index)
{
  getLogger() << "DummyChannelMaster::readRegister(" << index << ")" << InfoLogger::InfoLogger::endm;
  return 0;
}

void DummyChannelMaster::writeRegister(int index, uint32_t value)
{
  getLogger() << "DummyChannelMaster::writeRegister(index:" << index << ", value:" << value << ")"
      << InfoLogger::InfoLogger::endm;
}

CardType::type DummyChannelMaster::getCardType()
{
  return CardType::Dummy;
}

int DummyChannelMaster::getTransferQueueAvailable()
{
  return 0;
}

int DummyChannelMaster::getReadyQueueSize()
{
  return 0;
}

boost::optional<std::string> DummyChannelMaster::getFirmwareInfo()
{
  return std::string("Dummy");
}

} // namespace roc
} // namespace AliceO2
