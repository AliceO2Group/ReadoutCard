#include "ChannelMaster.h"
#include <iostream>
#include "ChannelPaths.h"

namespace AliceO2 {
namespace Rorc {

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
using std::cout;
using std::endl;

static constexpr int BUFFER_INDEX_PAGES = 0;

int ChannelMaster::getBufferId(int index)
{
  int n = dmaBuffersPerChannel;

  if (index >= n || index < 0) {
    ALICEO2_RORC_THROW_EXCEPTION("Tried to get buffer ID using invalid index");
  }

  return channelNumber * n + index;
}

ChannelMaster::ChannelMaster(int serial, int channel, const ChannelParameters& params, int dmaBuffersPerChannel)
 : serialNumber(serial),
   channelNumber(channel),
   dmaBuffersPerChannel(dmaBuffersPerChannel),
   sharedData(
       ChannelPaths::lock(serial, channel),
       ChannelPaths::state(serial, channel),
       sharedDataSize(),
       sharedDataName(),
       FileSharedObject::find_or_construct),
   pdaDevice(serialNumber),
   pdaBar(pdaDevice.getPciDevice(), channel),
   mappedFilePages(
       ChannelPaths::pages(serial, channel).c_str(),
       params.dma.bufferSize),
   bufferPages(
       pdaDevice.getPciDevice(),
       mappedFilePages.getAddress(),
       mappedFilePages.getSize(),
       getBufferId(BUFFER_INDEX_PAGES))
{
  // Initialize (if needed) the shared data
  const auto& sd = sharedData.get();

  if (sd->initializationState == InitializationState::INITIALIZED) {
   cout << "Shared channel state already initialized" << endl;
  }
  else {
   if (sd->initializationState == InitializationState::UNKNOWN) {
     cout << "Warning: unknown shared channel state. Proceeding with initialization" << endl;
   }
   cout << "Initializing shared channel state" << endl;
   sd->reset(params);
  }
}

ChannelMaster::~ChannelMaster()
{
}

ChannelMaster::SharedData::SharedData()
    : dmaState(DmaState::UNKNOWN), initializationState(InitializationState::UNKNOWN)
{
}

void ChannelMaster::SharedData::reset(const ChannelParameters& params)
{
  this->params = params;
  initializationState = InitializationState::INITIALIZED;
  dmaState = DmaState::STOPPED;
}

const ChannelParameters& ChannelMaster::getParams()
{
  return sharedData.get()->getParams();
}

const ChannelParameters& ChannelMaster::SharedData::getParams()
{
  return params;
}

ChannelMaster::InitializationState::type ChannelMaster::SharedData::getState()
{
  return initializationState;
}

void ChannelMaster::startDma()
{
  if (sharedData.get()->dmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (sharedData.get()->dmaState == DmaState::STARTED) {
    cout << "Warning: DMA already started. Ignoring startDma() call" << endl;
    return;
  }

  deviceStartDma();

  sharedData.get()->dmaState = DmaState::STARTED;
}

void ChannelMaster::stopDma()
{
  if (sharedData.get()->dmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (sharedData.get()->dmaState == DmaState::STOPPED) {
    cout << "Warning: DMA already stopped. Ignoring stopDma() call" << endl;
    return;
  }

  deviceStopDma();

  sharedData.get()->dmaState = DmaState::STOPPED;
}

uint32_t ChannelMaster::readRegister(int index)
{
  // TODO Range check
  return pdaBar.getUserspaceAddressU32()[index];
}

void ChannelMaster::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  pdaBar.getUserspaceAddressU32()[index] = value;
}

Page ChannelMaster::getPage(const PageHandle& handle)
{
  return Page(pageAddresses[handle.index].user);
}

void ChannelMaster::markPageAsRead(const PageHandle& handle)
{
  if (pageWasReadOut[handle.index]) {
    ALICEO2_RORC_THROW_EXCEPTION("Page was already marked as read");
  }
  pageWasReadOut[handle.index] = true;
}

} // namespace Rorc
} // namespace AliceO2
