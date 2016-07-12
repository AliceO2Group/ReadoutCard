///
/// \file ChannelMaster.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

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

static constexpr int CHANNELMASTER_DMA_BUFFERS_PER_CHANNEL = 1;
static constexpr int BUFFER_INDEX_PAGES = 0;

int ChannelMaster::getBufferId(int index)
{
  int n = dmaBuffersPerChannel;

  if (index >= n || index < 0) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_generic_message("Tried to get buffer ID using invalid index"));

  }

  return channelNumber * n + index;
}

void ChannelMaster::validateParameters(const ChannelParameters& ps)
{
  if (ps.dma.bufferSize % (2 * 1024 * 1024) != 0) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_generic_message("Parameter 'dma.bufferSize' not a multiple of 2 mebibytes"));
  }

  if (ps.generator.dataSize > ps.dma.pageSize) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_generic_message("Parameter 'generator.dataSize' greater than 'dma.pageSize'"));
  }

  if ((ps.dma.bufferSize % ps.dma.pageSize) != 0) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
            << errinfo_rorc_generic_message("DMA buffer size not a multiple of 'dma.pageSize'"));
  }
}

ChannelMaster::ChannelMaster(int serial, int channel, const ChannelParameters& params, int additionalBuffers)
 : serialNumber(serial),
   channelNumber(channel),
   dmaBuffersPerChannel(additionalBuffers + CHANNELMASTER_DMA_BUFFERS_PER_CHANNEL),
   sharedData(
       ChannelPaths::lock(serial, channel),
       ChannelPaths::state(serial, channel),
       sharedDataSize(),
       sharedDataName(),
       FileSharedObject::find_or_construct),
   deviceFinder(serialNumber),
   pdaDevice(deviceFinder.getPciVendorId(), deviceFinder.getPciDeviceId()),
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
  validateParameters(params);

  // Initialize (if needed) the shared data
  const auto& sd = sharedData.get();

  if (sd->initializationState == InitializationState::INITIALIZED) {
   //cout << "Shared channel state already initialized" << endl;
  }
  else {
   if (sd->initializationState == InitializationState::UNKNOWN) {
     //cout << "Warning: unknown shared channel state. Proceeding with initialization" << endl;
   }
   //cout << "Initializing shared channel state" << endl;
   sd->initialize(params);
  }
}

ChannelMaster::~ChannelMaster()
{
}

ChannelMaster::SharedData::SharedData()
    : dmaState(DmaState::UNKNOWN), initializationState(InitializationState::UNKNOWN)
{
}

void ChannelMaster::SharedData::initialize(const ChannelParameters& params)
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

// Checks DMA state and forwards call to subclass if necessary
void ChannelMaster::startDma()
{
  if (sharedData.get()->dmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (sharedData.get()->dmaState == DmaState::STARTED) {
    cout << "Warning: DMA already started. Ignoring startDma() call" << endl;
  } else {
    deviceStartDma();
  }

  sharedData.get()->dmaState = DmaState::STARTED;
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMaster::stopDma()
{
  if (sharedData.get()->dmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (sharedData.get()->dmaState == DmaState::STOPPED) {
    cout << "Warning: DMA already stopped. Ignoring stopDma() call" << endl;
  } else {
    deviceStopDma();
  }

  sharedData.get()->dmaState = DmaState::STOPPED;
}

uint32_t ChannelMaster::readRegister(int index)
{
  // TODO Range check
  return pdaBar[index];
}

void ChannelMaster::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  pdaBar[index] = value;
}

} // namespace Rorc
} // namespace AliceO2
