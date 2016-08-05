///
/// \file ChannelMaster.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "ChannelMaster.h"
#include <iostream>
#include "ChannelPaths.h"
#include "Util.h"

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
  if (ps.dma.bufferSize % (2l * 1024l * 1024l) != 0) {
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

void ChannelMaster::constructorCommonPhaseOne()
{
  using namespace Util;

  makeParentDirectories(ChannelPaths::pages(serialNumber, channelNumber));
  makeParentDirectories(ChannelPaths::state(serialNumber, channelNumber));
  makeParentDirectories(ChannelPaths::fifo(serialNumber, channelNumber));
  makeParentDirectories(ChannelPaths::lock(serialNumber, channelNumber));

  resetSmartPtr(sharedData, ChannelPaths::lock(serialNumber, channelNumber),
      ChannelPaths::state(serialNumber, channelNumber), sharedDataSize(), sharedDataName(),
      FileSharedObject::find_or_construct);

  resetSmartPtr(interProcessMutex, bip::open_or_create,
      ChannelPaths::namedMutex(serialNumber, channelNumber).c_str());

  resetSmartPtr(mutexGuard, interProcessMutex.get());
}

void ChannelMaster::constructorCommonPhaseTwo()
{
  using Util::resetSmartPtr;

  resetSmartPtr(rorcDevice, serialNumber);

  resetSmartPtr(pdaBar, rorcDevice->getPciDevice(), channelNumber);
  barUserspace = pdaBar->getUserspaceAddressU32();

  resetSmartPtr(mappedFilePages, ChannelPaths::pages(serialNumber, channelNumber).c_str(),
      getSharedData().getParams().dma.bufferSize);

  resetSmartPtr(bufferPages, rorcDevice->getPciDevice(), mappedFilePages->getAddress(), mappedFilePages->getSize(),
      getBufferId(BUFFER_INDEX_PAGES));
}

ChannelMaster::ChannelMaster(int serial, int channel, int additionalBuffers)
    : serialNumber(serial), channelNumber(channel), dmaBuffersPerChannel(
        additionalBuffers + CHANNELMASTER_DMA_BUFFERS_PER_CHANNEL)
{
  constructorCommonPhaseOne();

  // Initialize (if needed) the shared data
  const auto& sd = sharedData->get();
  const auto& params = sd->getParams();

  if (sd->initializationState == InitializationState::INITIALIZED) {
    validateParameters(params);
  }
  else {
    BOOST_THROW_EXCEPTION(SharedStateException()
        << errinfo_rorc_generic_message(sd->initializationState == InitializationState::UNINITIALIZED ?
            "Uninitialized shared data state" : "Unknown shared data state")
        << errinfo_rorc_shared_lock_file(ChannelPaths::lock(serialNumber, channelNumber).string())
        << errinfo_rorc_shared_state_file(ChannelPaths::state(serialNumber, channelNumber).string())
        << errinfo_rorc_shared_buffer_file(ChannelPaths::pages(serialNumber, channelNumber).string())
        << errinfo_rorc_shared_fifo_file(ChannelPaths::fifo(serialNumber, channelNumber).string())
        << errinfo_rorc_possible_causes({
            "Channel was never initialized with ChannelParameters",
            "Channel state file was corrupted",
            "Channel state file was used by incompatible library versions"}));
  }

  constructorCommonPhaseTwo();
}

ChannelMaster::ChannelMaster(int serial, int channel, const ChannelParameters& params, int additionalBuffers)
    : serialNumber(serial), channelNumber(channel), dmaBuffersPerChannel(
        additionalBuffers + CHANNELMASTER_DMA_BUFFERS_PER_CHANNEL)
{
  validateParameters(params);

  constructorCommonPhaseOne();

  // Initialize (if needed) the shared data
  const auto& sd = sharedData->get();
  if (sd->initializationState == InitializationState::INITIALIZED) {
    cout << "[LOG] Shared channel state already initialized" << endl;

    if (sd->getParams() == params) {
      cout << "[LOG] Shared state ChannelParameters equal to argument ChannelParameters" << endl;
    } else {
      cout << "[LOG] Shared state ChannelParameters different to argument ChannelParameters, reconfiguring channel"
          << endl;
      BOOST_THROW_EXCEPTION(CrorcException() << errinfo_rorc_generic_message(
                  "Automatic channel reconfiguration not yet supported. Clear channel state manually"));
    }
  } else {
    if (sd->initializationState == InitializationState::UNKNOWN) {
      cout << "[LOG] Warning: unknown shared channel state. Proceeding with initialization" << endl;
    }
    cout << "[LOG] Initializing shared channel state" << endl;
    sd->initialize(params);
  }

  constructorCommonPhaseTwo();
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

// Checks DMA state and forwards call to subclass if necessary
void ChannelMaster::startDma()
{
  const auto& sd = sharedData->get();

  if (sd->dmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (sd->dmaState == DmaState::STARTED) {
    cout << "Warning: DMA already started. Ignoring startDma() call" << endl;
  } else {
    deviceStartDma();
  }

  sd->dmaState = DmaState::STARTED;
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMaster::stopDma()
{
  const auto& sd = sharedData->get();

  if (sd->dmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (sd->dmaState == DmaState::STOPPED) {
    cout << "Warning: DMA already stopped. Ignoring stopDma() call" << endl;
  } else {
    deviceStopDma();
  }

  sd->dmaState = DmaState::STOPPED;
}

uint32_t ChannelMaster::readRegister(int index)
{
  // TODO Range check
  return barUserspace[index];
}

void ChannelMaster::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  barUserspace[index] = value;
}

} // namespace Rorc
} // namespace AliceO2
