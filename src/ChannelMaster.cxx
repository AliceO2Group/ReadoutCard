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

static constexpr size_t CHANNEL_SHARED_SIZE = 4l * 1024l; ///< 4k ought to be enough for anybody
static const char* CHANNEL_SHARED_NAME = "Channel";

/// DMA buffer ID for the pages
int getPagesBufferId(int channelNumber)
{
 return channelNumber * 2;
}

 /// DMA buffer ID for the readyFifo
int getFifoBufferId(int channelNumber)
{
 return (channelNumber * 2) + 1;
}

ChannelMaster::ChannelMaster(int serial, int channel, const ChannelParameters& params)
 : serialNumber(serial),
   channelNumber(channel),
   sharedData(
       ChannelPaths::lock(serial, channel),
       ChannelPaths::state(serial, channel),
       CHANNEL_SHARED_SIZE,
       CHANNEL_SHARED_NAME,
       FileSharedObject::find_or_construct),
   pdaDevice(serialNumber),
   pdaBar(pdaDevice.getPciDevice(), channel),
   mappedFilePages(
       ChannelPaths::pages(serial, channel).c_str(),
       params.dma.bufferSize),
   mappedFileFifo(
       ChannelPaths::fifo(serial, channel).c_str()),
   bufferPages(
       pdaDevice.getPciDevice(),
       mappedFilePages.getAddress(),
       mappedFilePages.getSize(),
       getPagesBufferId(channel)),
   bufferFifo(
       pdaDevice.getPciDevice(),
       mappedFileFifo.getAddress(),
       mappedFileFifo.getSize(),
       getFifoBufferId(channel))
{
 // Initialize (if needed) the shared data
 const auto& sd = sharedData.get();

 if (sd->getState() == SharedData::InitializationState::INITIALIZED) {
   cout << "Shared channel state already initialized" << endl;
 }
 else {
   if (sd->getState() == SharedData::InitializationState::UNKNOWN) {
     cout << "Warning: unknown shared channel state. Proceeding with initialization" << endl;
   }
   cout << "Initializing shared channel state" << endl;
   sd->reset(params);

   cout << "Clearing readyFifo" << endl;
   mappedFileFifo.get()->reset();
 }
}

ChannelMaster::~ChannelMaster()
{
}

ChannelMaster::SharedData::SharedData () : initializationState(InitializationState::UNKNOWN), fifoIndexIn(0), fifoIndexOut(0)
{
}

void ChannelMaster::SharedData::reset(const ChannelParameters& params)
{
  this->params = params;
  fifoIndexIn = 0;
  fifoIndexOut = 0;
  initializationState = InitializationState::INITIALIZED;
}

const ChannelParameters& ChannelMaster::SharedData::getParams()
{
  return params;
}

ChannelMaster::SharedData::InitializationState::type ChannelMaster::SharedData::getState()
{
  return initializationState;
}

} // namespace Rorc
} // namespace AliceO2
