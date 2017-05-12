/// \file DummyDmaChannel.cxx
/// \brief Implementation of the DummyDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DummyDmaChannel.h"
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

DummyDmaChannel::DummyDmaChannel(const Parameters& params)
    : DmaChannelBase(makeDummyDescriptor(), params, { 0, 1, 2, 3, 4, 5, 6, 7 })
{
  getLogger() << "DummyDmaChannel::DummyDmaChannel(channel:" << params.getChannelNumberRequired() << ")"
      << InfoLogger::InfoLogger::endm;
}

DummyDmaChannel::~DummyDmaChannel()
{
  getLogger() << "DummyDmaChannel::~DummyDmaChannel()" << InfoLogger::InfoLogger::endm;
}

void DummyDmaChannel::startDma()
{
  getLogger() << "DummyDmaChannel::startDma()" << InfoLogger::InfoLogger::endm;
}

void DummyDmaChannel::stopDma()
{
  getLogger() << "DummyDmaChannel::stopDma()" << InfoLogger::InfoLogger::endm;
}

void DummyDmaChannel::resetChannel(ResetLevel::type resetLevel)
{
  getLogger() << "DummyDmaChannel::resetCard(" << ResetLevel::toString(resetLevel) << ")"
      << InfoLogger::InfoLogger::endm;
}

uint32_t DummyDmaChannel::readRegister(int index)
{
  getLogger() << "DummyDmaChannel::readRegister(" << index << ")" << InfoLogger::InfoLogger::endm;
  return 0;
}

void DummyDmaChannel::writeRegister(int index, uint32_t value)
{
  getLogger() << "DummyDmaChannel::writeRegister(index:" << index << ", value:" << value << ")"
      << InfoLogger::InfoLogger::endm;
}

CardType::type DummyDmaChannel::getCardType()
{
  return CardType::Dummy;
}

int DummyDmaChannel::getTransferQueueAvailable()
{
  return 0;
}

int DummyDmaChannel::getReadyQueueSize()
{
  return 0;
}

boost::optional<std::string> DummyDmaChannel::getFirmwareInfo()
{
  return std::string("Dummy");
}

} // namespace roc
} // namespace AliceO2
