/// \file ChannelMaster.cxx
/// \brief Implementation of the ChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

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

namespace FilesystemType {
const char* SHARED_MEMORY = "tmpfs";
const char* HUGEPAGE = "hugetlbfs";
}

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
  return mChannelNumber * dmaBuffersPerChannel + index;
}

ChannelParameters ChannelMaster::convertParameters(const Parameters::Map& map)
{
  ChannelParameters cp;

  auto require = [&](const std::string& key) {
    if (!map.count(key)) {
      BOOST_THROW_EXCEPTION(ParameterException() << errinfo_rorc_error_message("Parameter '" + key + "' is required"));
    }
  };

  using namespace Parameters::Keys;

  require(dmaBufferSize());
  Util::lexicalCast(map.at(dmaBufferSize()), cp.dma.bufferSize);

  require(dmaPageSize());
  Util::lexicalCast(map.at(dmaPageSize()), cp.dma.pageSize);

  if (map.count(generatorEnabled())) {
    cp.generator.useDataGenerator = true;

    if (map.count(generatorDataSize())) {
      Util::lexicalCast(map.at(generatorDataSize()), cp.generator.dataSize);
    }

    if (map.count(generatorLoopbackMode())) {
      try {
        cp.generator.loopbackMode = LoopbackMode::fromString(map.at(generatorLoopbackMode()));
      } catch (const std::out_of_range& e) {
        BOOST_THROW_EXCEPTION(InvalidOptionValueException()
            << errinfo_rorc_error_message("Invalid value for parameter '" + generatorLoopbackMode() + "'"));
      }
    }
  }

  return cp;
}

void ChannelMaster::validateParameters(const ChannelParameters& cp)
{
  if (cp.dma.bufferSize % (2l * 1024l * 1024l) != 0) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("Parameter 'dma.bufferSize' not a multiple of 2 mebibytes"));
  }

  if (cp.generator.dataSize > cp.dma.pageSize) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("Parameter 'generator.dataSize' greater than 'dma.pageSize'"));
  }

  if ((cp.dma.bufferSize % cp.dma.pageSize) != 0) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("DMA buffer size not a multiple of 'dma.pageSize'"));
  }
}

void ChannelMaster::constructorCommonPhaseOne()
{
  using namespace Util;

  ChannelPaths paths(mCardType, mSerialNumber, mChannelNumber);

  // Create parent directories
  for (const auto& p : {paths.pages(), paths.state(), paths.fifo(), paths.lock()}) {
    makeParentDirectories(p);
  }

  // Check file system types
  {
    using namespace FilesystemType;
    assertFileSystemType(paths.state().parent_path(), {SHARED_MEMORY, HUGEPAGE}, "shared state");
    assertFileSystemType(paths.pages().parent_path(), {HUGEPAGE}, "DMA buffer");
  }

  // Get lock on shared data
  resetSmartPtr(mInterprocessLock, paths.lock(), paths.namedMutex());

  // Initialize file system objects
  resetSmartPtr(mSharedData, paths.state(), getSharedDataSize(), getSharedDataName().c_str(),
      FileSharedObject::find_or_construct);
}

void ChannelMaster::constructorCommonPhaseTwo()
{
  using Util::resetSmartPtr;

  resetSmartPtr(mRorcDevice, mSerialNumber);

  resetSmartPtr(mPdaBar, mRorcDevice->getPciDevice(), mChannelNumber);

  resetSmartPtr(mMappedFilePages, ChannelPaths(mCardType, mSerialNumber, mChannelNumber).pages().c_str(),
      getSharedData().getParams().dma.bufferSize);

  resetSmartPtr(mBufferPages, mRorcDevice->getPciDevice(), mMappedFilePages->getAddress(), mMappedFilePages->getSize(),
      getBufferId(BUFFER_INDEX_PAGES));
}

ChannelMaster::ChannelMaster(CardType::type cardType, int serial, int channel, int additionalBuffers)
    : mCardType(cardType), mSerialNumber(serial), mChannelNumber(channel), dmaBuffersPerChannel(
        additionalBuffers + CHANNELMASTER_DMA_BUFFERS_PER_CHANNEL)
{
  constructorCommonPhaseOne();

  // Initialize (if needed) the shared data
  const auto& sd = mSharedData->get();
  const auto& params = sd->getParams();

  if (sd->mInitializationState == InitializationState::INITIALIZED) {
    validateParameters(params);
  }
  else {
    ChannelPaths paths(cardType, mSerialNumber, mChannelNumber);
    BOOST_THROW_EXCEPTION(SharedStateException()
        << errinfo_rorc_error_message(sd->mInitializationState == InitializationState::UNINITIALIZED ?
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

ChannelMaster::ChannelMaster(CardType::type cardType, int serial, int channel, const Parameters::Map& paramMap,
    int additionalBuffers)
    : mCardType(cardType), mSerialNumber(serial), mChannelNumber(channel), dmaBuffersPerChannel(
        additionalBuffers + CHANNELMASTER_DMA_BUFFERS_PER_CHANNEL)
{
  auto params = convertParameters(paramMap);
  validateParameters(params);
  constructorCommonPhaseOne();

  // Initialize (if needed) the shared data
  const auto& sd = mSharedData->get();
  if (sd->mInitializationState == InitializationState::INITIALIZED) {
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
    if (sd->mInitializationState == InitializationState::UNKNOWN) {
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
    : mDmaState(DmaState::UNKNOWN), mInitializationState(InitializationState::UNKNOWN)
{
}

void ChannelMaster::SharedData::initialize(const ChannelParameters& params)
{
  this->mParams = params;
  mInitializationState = InitializationState::INITIALIZED;
  mDmaState = DmaState::STOPPED;
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMaster::startDma()
{
  const auto& sd = mSharedData->get();

  if (sd->mDmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (sd->mDmaState == DmaState::STARTED) {
    cout << "Warning: DMA already started. Ignoring startDma() call" << endl;
  } else {
    deviceStartDma();
  }

  sd->mDmaState = DmaState::STARTED;
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMaster::stopDma()
{
  const auto& sd = mSharedData->get();

  if (sd->mDmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (sd->mDmaState == DmaState::STOPPED) {
    cout << "Warning: DMA already stopped. Ignoring stopDma() call" << endl;
  } else {
    deviceStopDma();
  }

  sd->mDmaState = DmaState::STOPPED;
}

uint32_t ChannelMaster::readRegister(int index)
{
  // TODO Range check
  return mPdaBar->getUserspaceAddressU32()[index];
}

void ChannelMaster::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  mPdaBar->getUserspaceAddressU32()[index] = value;
}

} // namespace Rorc
} // namespace AliceO2
