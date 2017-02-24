/// \file ChannelMasterBase.cxx
/// \brief Implementation of the ChannelMasterBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelMasterBase.h"
#include <iostream>
#include "ChannelPaths.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/System.h"
#include "Visitor.h"

namespace AliceO2 {
namespace Rorc {

namespace b = boost;
namespace bfs = boost::filesystem;

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
  std::tie(found, type) = Utilities::isFileSystemTypeAnyOf(path, validTypes);

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
        << ErrorInfo::Message(oss.str())
        << ErrorInfo::FileName(path.c_str())
        << ErrorInfo::FilesystemType(type));
  }
}

} // Anonymous namespace

void ChannelMasterBase::checkChannelNumber(const AllowedChannels& allowedChannels)
{
  if (!allowedChannels.count(mChannelNumber)) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << ErrorInfo::Message("Channel number not supported")
        << ErrorInfo::ChannelNumber(mChannelNumber));
  }
}

ChannelParameters ChannelMasterBase::convertParameters(const Parameters& map)
{
  ChannelParameters cp;
//  cp.dma.bufferSize = map.getDmaBufferSize().get_value_or(4*1024*1024);
  cp.dma.pageSize = map.getDmaPageSize().get_value_or(8*1024);
  cp.generator.useDataGenerator = map.getGeneratorEnabled().get_value_or(true);
  cp.generator.dataSize = map.getGeneratorDataSize().get_value_or(cp.dma.pageSize);
  cp.generator.loopbackMode = map.getGeneratorLoopback().get_value_or(LoopbackMode::Rorc);
  return cp;
}

void ChannelMasterBase::validateParameters(const ChannelParameters& cp)
{
  // TODO
//  if (cp.dma.bufferSize % (2l * 1024l * 1024l) != 0) {
//    BOOST_THROW_EXCEPTION(InvalidParameterException()
//        << ErrorInfo::Message("Parameter 'dma.bufferSize' not a multiple of 2 mebibytes"));
//  }

  if (cp.generator.dataSize > cp.dma.pageSize) {
    BOOST_THROW_EXCEPTION(InvalidParameterException()
        << ErrorInfo::Message("Parameter 'generator.dataSize' greater than 'dma.pageSize'"));
  }

  // TODO
//  if ((cp.dma.bufferSize % cp.dma.pageSize) != 0) {
//    BOOST_THROW_EXCEPTION(InvalidParameterException()
//        << ErrorInfo::Message("DMA buffer size not a multiple of 'dma.pageSize'"));
//  }
}

ChannelMasterBase::ChannelMasterBase(CardType::type cardType, const Parameters& parameters, int serialNumber,
    const AllowedChannels& allowedChannels)
    : mCardType(cardType),
      mSerialNumber(serialNumber),
      mChannelNumber(parameters.getChannelNumberRequired())
{
  using namespace Utilities;

  // Check the channel number is allowed
  checkChannelNumber(allowedChannels);

  // Convert parameters to internal representation
  mChannelParameters = convertParameters(parameters);
  validateParameters(mChannelParameters);

  auto paths = getPaths();

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

  // Get lock
  log("Getting master lock", InfoLogger::InfoLogger::Debug);
  resetSmartPtr(mInterprocessLock, paths.lock(), paths.namedMutex());
  log("Acquired master lock", InfoLogger::InfoLogger::Debug);

  if (auto bufferParameters = parameters.getBufferParameters()) {
    // Create appropriate BufferProvider subclass
    mBufferProvider = Visitor::apply<std::unique_ptr<BufferProvider>>(*bufferParameters,
        [&](BufferParameters::Memory parameters){ return std::make_unique<BufferProviderMemory>(parameters); },
        [&](BufferParameters::File parameters){ return std::make_unique<BufferProviderFile>(parameters); });
  } else {
    BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("ChannelMaster requires BufferParameters"));
  }
}

ChannelMasterBase::~ChannelMasterBase()
{
  log("Releasing master lock", InfoLogger::InfoLogger::Debug);
}

} // namespace Rorc
} // namespace AliceO2
