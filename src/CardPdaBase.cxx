///
/// \file CardPdaBase.cxx
/// \author Pascal Boeschoten
///

#include "CardPdaBase.h"
#include <sys/mman.h>
#include <iostream>
#include <string>
#include <memory>
#include <boost/lexical_cast.hpp>
#include "RorcException.h"

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

void* openOrCreateSharedMemory(std::string path, size_t size);

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

  if (cd.params().dma.useSharedMemory) {
    initDmaBufferSharedMemory(cd);
  } else {
    initDmaBufferKernelMemory(cd);
  }

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

  if (cd.params().dma.useSharedMemory) {
    // TODO Clean up shared mem files? Or leave them around for debugging?
    auto path = getSharedMemoryPagesFilePath(cd);
    remove(path.c_str());
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

void CardPdaBase::initDmaBufferKernelMemory(ChannelData& cd)
{
  // Allocating buffer, getting the corresponding DMABuffer struct
  if (PciDevice_allocDMABuffer(pciDevice, cd.channel, cd.params().dma.bufferSize, &cd.dmaBuffer) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to allocate DMA buffer");
  }

  if (cd.dmaBuffer == nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to allocate DMA buffer");
  }

  // Mapping the start of the buffer to userspace.
  if (DMABuffer_getMap(cd.dmaBuffer, (void**) &cd.mappedBuffer) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to map DMA buffer");
  }
}


std::string CardPdaBase::getSharedMemoryPagesFilePath(ChannelData& cd)
{
  std::stringstream ss;
  ss << "/mnt/hugetlbfs/"
    << "Alice02_RORC_" << serialNumber
    << "_Channel_" << cd.channel
    << "_Pages";
  return ss.str();
}

void CardPdaBase::initDmaBufferSharedMemory(ChannelData& cd)
{
  size_t bufferSize = cd.params().dma.bufferSize;

  void* sharedMemoryPointer = openOrCreateSharedMemory(getSharedMemoryPagesFilePath(cd), bufferSize);

  // Tell PDA we're using our already allocated userspace buffer.
  if (PciDevice_registerDMABuffer(pciDevice, cd.channel, sharedMemoryPointer, bufferSize,
      &cd.dmaBuffer) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to register external DMA buffer");
  }

  // Mapping the start of the buffer to userspace.
  // Note that the immediately visible result of this should be identical to just assigning the shared memory pointer
  // to mappedBuffer ourselves, since the buffer is already in userspace. But perhaps PDA does some extra magic behind
  // the scenes that we don't want to miss out on.
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

void* openOrCreateSharedMemory(std::string path, size_t size)
{
  void* ptr;

  // Open or create the file
  // TODO Figure out appropriate file permissions. Most likely, clients should not be able to access until the server
  // has finished initializing
  int file = open(path.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  if (file == -1) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to open shared memory file");
  }

  // Resizing the file
  if (ftruncate(file, size)) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to resize shared memory file");
  }

  // Mapping the file
  auto mmapProtect = PROT_READ | PROT_WRITE;
  auto mmapFlags = MAP_SHARED | MAP_LOCKED;
  auto mmapFile = file;
  auto mmapOffset = 0;
  ptr = mmap(NULL, size, mmapProtect, mmapFlags, mmapFile, mmapOffset);

  if (ptr == MAP_FAILED) {
    cout << strerror(errno) << endl;
    ALICEO2_RORC_THROW_EXCEPTION("Failed to map shared memory");
  }

  return ptr;
}

} // namespace Rorc
} // namespace AliceO2
