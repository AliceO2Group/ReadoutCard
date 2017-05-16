/// \file DummyDmaChannel.cxx
/// \brief Implementation of the DummyDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DummyDmaChannel.h"
#include <chrono>
#include <random>
#include "ReadoutCard/ChannelFactory.h"

namespace AliceO2 {
namespace roc {
namespace {
CardDescriptor makeDummyDescriptor()
{
  return {CardType::Dummy, ChannelFactory::DUMMY_SERIAL_NUMBER, PciId {"dummy", "dummy"}, PciAddress {0,0,0}};
}

constexpr size_t TRANSFER_QUEUE_SIZE = 16;
constexpr size_t READY_QUEUE_SIZE = 32;
}

constexpr auto endm = InfoLogger::InfoLogger::StreamOps::endm;

DummyDmaChannel::DummyDmaChannel(const Parameters& params)
    : DmaChannelBase(makeDummyDescriptor(), params, { 0, 1, 2, 3, 4, 5, 6, 7 }),
      mTransferQueue(TRANSFER_QUEUE_SIZE), mReadyQueue(READY_QUEUE_SIZE)
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
  mTransferQueue.clear();
  mReadyQueue.clear();
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
  return mTransferQueue.capacity() - mTransferQueue.size();
}

int DummyDmaChannel::getReadyQueueSize()
{
  return mReadyQueue.size();
}

boost::optional<std::string> DummyDmaChannel::getFirmwareInfo()
{
  return std::string("Dummy");
}

void DummyDmaChannel::pushSuperpage(Superpage superpage)
{
  if (getTransferQueueAvailable() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
  }

  if (superpage.getSize() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, size == 0"));
  }

  if (!Utilities::isMultiple(superpage.getSize(), size_t(32*1024))) {
    BOOST_THROW_EXCEPTION(Exception()
                            << ErrorInfo::Message("Could not enqueue superpage, size not a multiple of 32 KiB"));
  }

  if ((superpage.getOffset() + superpage.getSize()) > getBufferProvider().getSize()) {
    BOOST_THROW_EXCEPTION(Exception()
                            << ErrorInfo::Message("Superpage out of range"));
  }

  if ((superpage.getOffset() % 4) != 0) {
    BOOST_THROW_EXCEPTION(Exception()
                            << ErrorInfo::Message("Superpage offset not 32-bit aligned"));
  }

  mTransferQueue.push_back(superpage);
}

Superpage DummyDmaChannel::getSuperpage()
{
  return mReadyQueue.front();
}

Superpage DummyDmaChannel::popSuperpage()
{
  if (mReadyQueue.empty()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, ready queue was empty"));
  }

  auto superpage = mReadyQueue.front();
  mReadyQueue.pop_front();
  return superpage;
}

void DummyDmaChannel::fillSuperpages()
{
  size_t pushQueueSize = mTransferQueue.size();
  for (size_t i = 0; i < pushQueueSize; ++i) {
    if (mReadyQueue.full()) {
      break;
    }
    mTransferQueue.front().ready = true;
    mTransferQueue.front().received = mTransferQueue.front().size;
    mReadyQueue.push_back(mTransferQueue.front());
    mTransferQueue.pop_front();
  }
}

boost::optional<float> DummyDmaChannel::getTemperature()
{
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
  std::mt19937 engine {seconds};
  std::uniform_real_distribution<float> distribution {37, 43};
  return {distribution(engine)};
}

} // namespace roc
} // namespace AliceO2
