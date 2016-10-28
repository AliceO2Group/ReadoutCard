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

void ChannelMaster::checkChannelNumber(const AllowedChannels& allowedChannels)
{
  if (!allowedChannels.count(mChannelNumber)) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("Channel number not supported")
        << errinfo_rorc_channel_number(mChannelNumber));
  }
}

ChannelParameters ChannelMaster::convertParameters(const Parameters& map)
{
  ChannelParameters cp;
  cp.dma.bufferSize = map.get<Parameters::DmaBufferSize>().get_value_or(4*1024*1024);
  cp.dma.pageSize = map.get<Parameters::DmaPageSize>().get_value_or(8*1024);
  cp.generator.useDataGenerator = map.get<Parameters::GeneratorEnabled>().get_value_or(true);
  cp.generator.dataSize = map.get<Parameters::GeneratorDataSize>().get_value_or(cp.dma.pageSize);
  cp.generator.loopbackMode = map.get<Parameters::GeneratorLoopbackMode>().get_value_or(LoopbackMode::Rorc);
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

ChannelMaster::ChannelMaster(CardType::type cardType, const Parameters& parameters, int additionalBuffers,
    const AllowedChannels& allowedChannels)
    : mSerialNumber(parameters.getRequired<Parameters::SerialNumber>()),
      mChannelNumber(parameters.getRequired<Parameters::ChannelNumber>()),
      dmaBuffersPerChannel(additionalBuffers + CHANNELMASTER_DMA_BUFFERS_PER_CHANNEL),
      mDmaState(DmaState::STOPPED)
{
  using namespace Util;

  mChannelParameters = convertParameters(parameters);
  validateParameters(mChannelParameters);

  checkChannelNumber(allowedChannels);

  ChannelPaths paths(cardType, mSerialNumber, mChannelNumber);

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

  resetSmartPtr(mRorcDevice, mSerialNumber);

  resetSmartPtr(mPdaBar, mRorcDevice->getPciDevice(), mChannelNumber);

  resetSmartPtr(mMappedFilePages, ChannelPaths(cardType, mSerialNumber, mChannelNumber).pages().string(),
      getChannelParameters().dma.bufferSize);

  resetSmartPtr(mBufferPages, mRorcDevice->getPciDevice(), mMappedFilePages->getAddress(), mMappedFilePages->getSize(),
      getBufferId(BUFFER_INDEX_PAGES));
}

ChannelMaster::~ChannelMaster()
{
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMaster::startDma()
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (mDmaState == DmaState::STARTED) {
    cout << "Warning: DMA already started. Ignoring startDma() call" << endl;
  } else {
    deviceStartDma();
  }
  mDmaState = DmaState::STARTED;
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMaster::stopDma()
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    cout << "Warning: Unknown DMA state" << endl;
  } else if (mDmaState == DmaState::STOPPED) {
    cout << "Warning: DMA already stopped. Ignoring stopDma() call" << endl;
  } else {
    deviceStopDma();
  }
  mDmaState = DmaState::STOPPED;
}

void ChannelMaster::resetChannel(ResetLevel::type resetLevel)
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message("Reset channel failed: DMA in unknown state"));
  }
  if (mDmaState != DmaState::STOPPED) {
    BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message("Reset channel failed: DMA was not stopped"));
  }

  deviceResetChannel(resetLevel);
}

uint32_t ChannelMaster::readRegister(int index)
{
  return mPdaBar->getUserspaceAddressU32()[index];
}

void ChannelMaster::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  mPdaBar->getUserspaceAddressU32()[index] = value;
}

void ChannelMaster::setLogLevel(InfoLogger::InfoLogger::Severity severity)
{
  mLogLevel = severity;
}

} // namespace Rorc
} // namespace AliceO2
