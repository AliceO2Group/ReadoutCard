///
/// \file CardPdaBase.cxx
/// \author Pascal Boeschoten
///

#include "CardPdaBase.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "RorcException.h"

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

CardPdaBase::ChannelData::ChannelData(int channel, const ChannelParameters& parameters)
    : channel(channel), parameters(parameters), dmaBuffer(nullptr), mappedBuffer(nullptr)
{
}

CardPdaBase::CardPdaBase(DeviceOperator* deviceOperator, PciDevice* pciDevice, int serialNumber, int amountOfChannels)
    : channelDataVector(amountOfChannels),
      channelOpen(amountOfChannels, false),
      deviceOperator(deviceOperator),
      pciDevice(pciDevice),
      serialNumber(serialNumber)
{
  if (this->deviceOperator == nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to construct CardPdaBase: DeviceOperator is null");
  }

  if (this->pciDevice == nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to construct CardPdaBase: PCIDevice is null");
  }
}

CardPdaBase::~CardPdaBase()
{
  if(DeviceOperator_delete(deviceOperator, PDA_DELETE) != PDA_SUCCESS){
    ALICEO2_RORC_THROW_EXCEPTION("Failed to delete device operator");
  }
}

bool CardPdaBase::isChannelOpen(int channel)
{
  return channelOpen[channel];
}

void CardPdaBase::setChannelOpen(int channel, bool open)
{
  channelOpen[channel] = open;
}

/// Opens a channel on a card
void CardPdaBase::openChannel(int channel, const ChannelParameters& channelParameters)
{
  validateChannelParameters(channelParameters);

  auto& cd = channelDataVector.at(channel);

  if (isChannelOpen(channel)) {
    ALICEO2_RORC_THROW_EXCEPTION("Channel already open");
  }

  // Reset data
  cd = ChannelData(channel, channelParameters);
  setChannelOpen(channel, true);

  cd.pageWasReadOut.resize(cd.params().fifo.entries, true);

  initBar(cd);
  initDmaBuffer(cd);
  initDmaBufferMap(cd);
  initScatterGatherList(cd);

  // Let subclass handle the rest
  deviceOpenDmaChannel(channel);
}

/// Closes channel
void CardPdaBase::closeChannel(int channel)
{
  // Let subclass handle closing
  deviceCloseDmaChannel(channel);

  auto& cd = channelDataVector.at(channel);

  if (!isChannelOpen(channel)) {
    ALICEO2_RORC_THROW_EXCEPTION("Channel already closed");
  }

  // Mark channel as closed and reset its data
  setChannelOpen(channel, false);
  cd = ChannelData(channel);
}

CardPdaBase::ChannelData& CardPdaBase::getChannelData(int channel)
{
  if (channel < 0 || channel >= getNumberOfChannels()) {
    ALICEO2_RORC_THROW_EXCEPTION("Channel number '" + boost::lexical_cast<std::string>(channel) + "' invalid");
  }

  ChannelData& cd = channelDataVector.at(channel);

  if (!isChannelOpen(channel)) {
    ALICEO2_RORC_THROW_EXCEPTION("Channel number '" + boost::lexical_cast<std::string>(channel) + "' was not open");
  }

  return cd;
}

DMABuffer_SGNode* CardPdaBase::getScatterGatherList(int channel)
{
  auto& cd = getChannelData(channel);
  DMABuffer_SGNode* sgList;

  if (cd.dmaBuffer == nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("Could not get scatter-gather list: DMA Buffer is null");
  }

  if (DMABuffer_getSGList(cd.dmaBuffer, &sgList) != PDA_SUCCESS ){
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get scatter-gather list");
  }

  return sgList;
}

int CardPdaBase::getNumberOfPages(int channel)
{
  return getChannelData(channel).sglWrapper->pages.size();
}

volatile void* CardPdaBase::getMappedMemory(int channel)
{
  auto& cd = getChannelData(channel);

  if (cd.mappedBuffer == nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("Buffer was not mapped");
  }

  return cd.mappedBuffer;
}


void CardPdaBase::initBar(ChannelData& cd)
{
  cd.bar = BarWrapper(pciDevice, cd.channel);
}

void CardPdaBase::initDmaBuffer(ChannelData& cd)
{
  // Allocating buffer, getting the corresponding DMABuffer struct
  if (PciDevice_allocDMABuffer(pciDevice, cd.channel, cd.params().dma.getBufferSizeBytes(), &cd.dmaBuffer) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to allocate DMA buffer");
  }

  if (cd.dmaBuffer == nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to allocate DMA buffer");
  }
}

void CardPdaBase::initDmaBufferMap(ChannelData& cd)
{
  // Mapping the start of the buffer to userspace.
  if (DMABuffer_getMap(cd.dmaBuffer, (void**) &cd.mappedBuffer) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to map DMA buffer");
  }
}

void CardPdaBase::initScatterGatherList(ChannelData& cd)
{
  cd.sglWrapper.reset(
      new SglWrapper(getScatterGatherList(cd.channel), cd.params().dma.pageSize, cd.params().fifo.getFullOffset(),
          cd.params().fifo.entries));
}

} // namespace Rorc
} // namespace AliceO2
