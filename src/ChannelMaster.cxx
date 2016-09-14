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

namespace {

/// Throws if the file system type of the given file/directory is not one of the given valid types
void assertFileSystemType(const bfs::path& path, const std::set<std::string>& validTypes, const std::string& name)
{
  bool found;
  std::string type;
  std::tie(found, type) = Util::isFileSystemTypeAnyOf(path, validTypes);

  if (!found) {
    std::ostringstream oss;
    oss << "File-backed shared memory for '" << name << "' file system type invalid (supported: ";
    for (auto i = validTypes.begin(); i != validTypes.end(); i++) {
      oss << *i;
      if (i != validTypes.end()) {
        oss << ",";
      }
    }
    oss << ")";

    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message(oss.str())
        << errinfo_rorc_filename(path.c_str())
        << errinfo_rorc_filesystem_type(type));
  }
}

static constexpr int CHANNELMASTER_DMA_BUFFERS_PER_CHANNEL = 1;
static constexpr int BUFFER_INDEX_PAGES = 0;

} // Anonymous namespace

int ChannelMaster::getBufferId(int index) const
{
  if (index >= dmaBuffersPerChannel || index < 0) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("Tried to get buffer ID using invalid index"));
  }
  return channelNumber * dmaBuffersPerChannel + index;
}

void ChannelMaster::validateParameters(const ChannelParameters& ps)
{
  if (ps.dma.bufferSize % (2l * 1024l * 1024l) != 0) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("Parameter 'dma.bufferSize' not a multiple of 2 mebibytes"));
  }

  if (ps.generator.dataSize > ps.dma.pageSize) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("Parameter 'generator.dataSize' greater than 'dma.pageSize'"));
  }

  if ((ps.dma.bufferSize % ps.dma.pageSize) != 0) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("DMA buffer size not a multiple of 'dma.pageSize'"));
  }
}

void ChannelMaster::constructorCommonPhaseOne()
{
  using namespace Util;

  ChannelPaths paths(cardType, serialNumber, channelNumber);

  // Create parent directories
  for (auto& p : {paths.pages(), paths.state(), paths.fifo(), paths.lock()}) {
    makeParentDirectories(p);
  }

  // Check file system types
  assertFileSystemType(paths.state(), {"tmpfs", "hugetlbfs"}, "shared state");
  assertFileSystemType(paths.pages(), {"hugetlbfs"}, "DMA buffer");

  // Initialize file system objects
  resetSmartPtr(sharedData, paths.lock(), paths.state(), getSharedDataSize(), getSharedDataName().c_str(),
      FileSharedObject::find_or_construct);

  resetSmartPtr(interProcessMutex, bip::open_or_create, paths.namedMutex().c_str());

  resetSmartPtr(mutexGuard, interProcessMutex.get());
}

void ChannelMaster::constructorCommonPhaseTwo()
{
  using Util::resetSmartPtr;

  resetSmartPtr(rorcDevice, serialNumber);

  resetSmartPtr(pdaBar, rorcDevice->getPciDevice(), channelNumber);

  resetSmartPtr(mappedFilePages, ChannelPaths(cardType, serialNumber, channelNumber).pages().c_str(),
      getSharedData().getParams().dma.bufferSize);

  resetSmartPtr(bufferPages, rorcDevice->getPciDevice(), mappedFilePages->getAddress(), mappedFilePages->getSize(),
      getBufferId(BUFFER_INDEX_PAGES));
}

ChannelMaster::ChannelMaster(CardType::type cardType, int serial, int channel, int additionalBuffers)
    : cardType(cardType), serialNumber(serial), channelNumber(channel), dmaBuffersPerChannel(
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
    ChannelPaths paths(cardType, serialNumber, channelNumber);
    BOOST_THROW_EXCEPTION(SharedStateException()
        << errinfo_rorc_error_message(sd->initializationState == InitializationState::UNINITIALIZED ?
            "Uninitialized shared data state" : "Unknown shared data state")
        << errinfo_rorc_shared_lock_file(paths.lock().string())
        << errinfo_rorc_shared_state_file(paths.state().string())
        << errinfo_rorc_shared_buffer_file(paths.pages().string())
        << errinfo_rorc_shared_fifo_file(paths.fifo().string())
        << errinfo_rorc_possible_causes({
            "Channel was never initialized with ChannelParameters",
            "Channel state file was corrupted",
            "Channel state file was used by incompatible library versions"}));
  }

  constructorCommonPhaseTwo();
}

ChannelMaster::ChannelMaster(CardType::type cardType, int serial, int channel, const ChannelParameters& params,
    int additionalBuffers)
    : cardType(cardType), serialNumber(serial), channelNumber(channel), dmaBuffersPerChannel(
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
      BOOST_THROW_EXCEPTION(CrorcException() << errinfo_rorc_error_message(
          "Automatic channel reconfiguration not yet supported. Clear channel state manually")
          << errinfo_rorc_channel_number(channel)
          << errinfo_rorc_serial_number(serial));
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
  return pdaBar->getUserspaceAddressU32()[index];
}

void ChannelMaster::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  pdaBar->getUserspaceAddressU32()[index] = value;
}

} // namespace Rorc
} // namespace AliceO2
