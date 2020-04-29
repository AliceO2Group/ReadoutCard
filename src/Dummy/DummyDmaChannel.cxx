// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DummyDmaChannel.cxx
/// \brief Implementation of the DummyDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DummyDmaChannel.h"
#include <chrono>
#include <random>
#include "ReadoutCard/ChannelFactory.h"
#include "Visitor.h"

namespace AliceO2
{
namespace roc
{
namespace
{
CardDescriptor makeDummyDescriptor()
{
  return { CardType::Dummy, ChannelFactory::getDummySerialId(), PciId{ "dummy", "dummy" }, PciAddress{ 0, 0, 0 }, -1, -1 };
}

constexpr size_t TRANSFER_QUEUE_CAPACITY = 16;
constexpr size_t READY_QUEUE_CAPACITY = 32;
} // namespace

constexpr auto endm = InfoLogger::InfoLogger::StreamOps::endm;

DummyDmaChannel::DummyDmaChannel(const Parameters& params)
  : DmaChannelBase(makeDummyDescriptor(), const_cast<Parameters&>(params), { 0, 1, 2, 3, 4, 5, 6, 7 }),
    mTransferQueue(TRANSFER_QUEUE_CAPACITY),
    mReadyQueue(READY_QUEUE_CAPACITY)
{
  getLogger() << "DummyDmaChannel::DummyDmaChannel(channel:" << params.getChannelNumberRequired() << ")"
              << InfoLogger::InfoLogger::endm;

  if (auto bufferParameters = params.getBufferParameters()) {
    // Create appropriate BufferProvider subclass
    Visitor::apply(*bufferParameters,
                   [&](buffer_parameters::Memory parameters) { mBufferSize = parameters.size; },
                   [&](buffer_parameters::File parameters) { mBufferSize = parameters.size; },
                   [&](buffer_parameters::Null) { mBufferSize = 0; });
  } else {
    BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("DmaChannel requires buffer_parameters"));
  }
}

DummyDmaChannel::~DummyDmaChannel()
{
  getLogger() << "DummyDmaChannel::~DummyDmaChannel()" << endm;
}

void DummyDmaChannel::startDma()
{
  getLogger() << "DummyDmaChannel::startDma()" << endm;
  mTransferQueue.clear();
  mReadyQueue.clear();
}

void DummyDmaChannel::stopDma()
{
  getLogger() << "DummyDmaChannel::stopDma()" << endm;
}

void DummyDmaChannel::resetChannel(ResetLevel::type resetLevel)
{
  getLogger() << "DummyDmaChannel::resetCard(" << ResetLevel::toString(resetLevel) << ")"
              << endm;
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

bool DummyDmaChannel::pushSuperpage(Superpage superpage)
{
  if (getTransferQueueAvailable() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not push superpage, transfer queue was full"));
  }

  if (superpage.getSize() == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, size == 0"));
  }

  if (!Utilities::isMultiple(superpage.getSize(), size_t(32 * 1024))) {
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Could not enqueue superpage, size not a multiple of 32 KiB"));
  }

  if ((superpage.getOffset() + superpage.getSize()) > mBufferSize) {
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Superpage out of range"));
  }

  if ((superpage.getOffset() % 4) != 0) {
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Superpage offset not 32-bit aligned"));
  }

  mTransferQueue.push_back(superpage);
  return true;
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
    mTransferQueue.front().setReady(true);
    mTransferQueue.front().setReceived(mTransferQueue.front().getSize());
    mReadyQueue.push_back(mTransferQueue.front());
    mTransferQueue.pop_front();
  }
}

bool DummyDmaChannel::isTransferQueueEmpty()
{
  return mTransferQueue.empty();
}

bool DummyDmaChannel::isReadyQueueFull()
{
  return mReadyQueue.size() == READY_QUEUE_CAPACITY;
}

int32_t DummyDmaChannel::getDroppedPackets()
{
  return 0; // No dropped packets on the Dummy DMA Channel
}

boost::optional<int32_t> DummyDmaChannel::getSerial()
{
  return ChannelFactory::getDummySerialId().getSerial();
}

boost::optional<float> DummyDmaChannel::getTemperature()
{
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
                   .count();
  std::mt19937 engine{ static_cast<uint32_t>(seconds) };
  std::uniform_real_distribution<float> distribution{ 37, 43 };
  return { distribution(engine) };
}

PciAddress DummyDmaChannel::getPciAddress()
{
  return PciAddress(0, 0, 0);
}

int DummyDmaChannel::getNumaNode()
{
  return 0;
}

} // namespace roc
} // namespace AliceO2
