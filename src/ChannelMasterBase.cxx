/// \file ChannelMaster.cxx
/// \brief Implementation of the ChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelMasterBase.h"
#include <iostream>
#include "ChannelPaths.h"
#include "Pda/Pda.h"
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

int getBufferId(int channel)
{
  return channel;
}

} // Anonymous namespace

void ChannelMasterBase::checkChannelNumber(const AllowedChannels& allowedChannels)
{
  if (!allowedChannels.count(mChannelNumber)) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << errinfo_rorc_error_message("Channel number not supported")
        << errinfo_rorc_channel_number(mChannelNumber));
  }
}

ChannelParameters ChannelMasterBase::convertParameters(const Parameters& map)
{
  ChannelParameters cp;
  cp.dma.bufferSize = map.get<Parameters::DmaBufferSize>().get_value_or(4*1024*1024);
  cp.dma.pageSize = map.get<Parameters::DmaPageSize>().get_value_or(8*1024);
  cp.generator.useDataGenerator = map.get<Parameters::GeneratorEnabled>().get_value_or(true);
  cp.generator.dataSize = map.get<Parameters::GeneratorDataSize>().get_value_or(cp.dma.pageSize);
  cp.generator.loopbackMode = map.get<Parameters::GeneratorLoopbackMode>().get_value_or(LoopbackMode::Rorc);
  return cp;
}

void ChannelMasterBase::validateParameters(const ChannelParameters& cp)
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

ChannelMasterBase::ChannelMasterBase(CardType::type cardType, const Parameters& parameters,
    const AllowedChannels& allowedChannels, size_t fifoSize)
    : mDmaState(DmaState::STOPPED),
      mSerialNumber(parameters.getRequired<Parameters::SerialNumber>()),
      mChannelNumber(parameters.getRequired<Parameters::ChannelNumber>())
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
      getBufferId(mChannelNumber));

  partitionDmaBuffer(fifoSize, mChannelParameters.dma.pageSize);
}

ChannelMasterBase::~ChannelMasterBase()
{
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMasterBase::startDma()
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    log("Unknown DMA state");
  } else if (mDmaState == DmaState::STARTED) {
    log("DMA already started. Ignoring startDma() call");
  } else {
    deviceStartDma();
  }
  mDmaState = DmaState::STARTED;
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMasterBase::stopDma()
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    log("Unknown DMA state");
  } else if (mDmaState == DmaState::STOPPED) {
    log("Warning: DMA already stopped. Ignoring stopDma() call");
  } else {
    deviceStopDma();
  }
  mDmaState = DmaState::STOPPED;
}

void ChannelMasterBase::resetChannel(ResetLevel::type resetLevel)
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

uint32_t ChannelMasterBase::readRegister(int index)
{
  return mPdaBar->getRegister<uint32_t>(index * sizeof(uint32_t));
}

void ChannelMasterBase::writeRegister(int index, uint32_t value)
{
  mPdaBar->setRegister<uint32_t>(index * sizeof(uint32_t), value);
}

void ChannelMasterBase::setLogLevel(InfoLogger::InfoLogger::Severity severity)
{
  mLogLevel = severity;
}

void ChannelMasterBase::partitionDmaBuffer(size_t fifoSize, size_t pageSize)
{
  /// Amount of space reserved for the FIFO, we use multiples of the page size for uniformity
  size_t fifoSpace = ((fifoSize / pageSize) + 1) * pageSize;
  PageAddress fifoAddress;
  std::tie(fifoAddress, mPageAddresses) = Pda::partitionScatterGatherList(mBufferPages->getScatterGatherList(),
      fifoSpace, pageSize);
  mFifoAddressUser = const_cast<void*>(fifoAddress.user);
  mFifoAddressBus = const_cast<void*>(fifoAddress.bus);
}


} // namespace Rorc
} // namespace AliceO2
