/// \file ChannelMasterBase.cxx
/// \brief Implementation of the ChannelMasterBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelMasterBase.h"
#include <iostream>
#include "BufferProviderFile.h"
#include "BufferProviderMemory.h"
#include "ChannelPaths.h"
#include "Common/System.h"
#include "Utilities/SmartPointer.h"
#include "Visitor.h"

namespace AliceO2 {
namespace roc {

namespace b = boost;
namespace bfs = boost::filesystem;

void ChannelMasterBase::checkChannelNumber(const AllowedChannels& allowedChannels)
{
  if (!allowedChannels.count(mChannelNumber)) {
    std::ostringstream stream;
    stream << "Channel number not supported, must be one of: ";
    for (int c : allowedChannels) {
      stream << c << ' ';
    }
    BOOST_THROW_EXCEPTION(InvalidParameterException() << ErrorInfo::Message(stream.str())
        << ErrorInfo::ChannelNumber(mChannelNumber));
  }
}

ChannelMasterBase::ChannelMasterBase(CardDescriptor cardDescriptor, const Parameters& parameters,
    const AllowedChannels& allowedChannels)
    : mCardDescriptor(cardDescriptor), mChannelNumber(parameters.getChannelNumberRequired())
{
#ifndef NDEBUG
  log("Backend compiled without NDEBUG; performance may be severely degraded", InfoLogger::InfoLogger::Info);
#endif

  // Check the channel number is allowed
  checkChannelNumber(allowedChannels);

  // Create parent directories
  auto paths = getPaths();
  for (const auto& p : {paths.fifo(), paths.lock()}) {
    Common::System::makeParentDirectories(p);
  }

  // Get lock
  log("Getting channel lock", InfoLogger::InfoLogger::Debug);
  try {
    Utilities::resetSmartPtr(mInterprocessLock, paths.lock(), paths.namedMutex());
  }
  catch (const NamedMutexLockException& exception) {
    if (parameters.getForcedUnlockEnabled().get_value_or(false)) {
      log("Failed to acquire channel mutex. Forced unlock enabled - attempting cleanup and retry",
          InfoLogger::InfoLogger::Warning);
      // The user has indicated to the driver that it is safe to force the unlock. We do it by deleting the lock.
      boost::interprocess::named_mutex::remove(paths.namedMutex().c_str());
      // Try again...
      Utilities::resetSmartPtr(mInterprocessLock, paths.lock(), paths.namedMutex());
    } else {
      log("Failed to acquire lock", InfoLogger::InfoLogger::Debug);
      throw;
    }
  }
  log("Acquired channel lock", InfoLogger::InfoLogger::Debug);

  if (auto bufferParameters = parameters.getBufferParameters()) {
    // Create appropriate BufferProvider subclass
    mBufferProvider = Visitor::apply<std::unique_ptr<BufferProvider>>(*bufferParameters,
        [&](buffer_parameters::Memory parameters){ return std::make_unique<BufferProviderMemory>(parameters); },
        [&](buffer_parameters::File parameters){ return std::make_unique<BufferProviderFile>(parameters); });
  } else {
    BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("ChannelMaster requires buffer_parameters"));
  }
}

ChannelMasterBase::~ChannelMasterBase()
{
  log("Releasing master lock", InfoLogger::InfoLogger::Debug);
}

} // namespace roc
} // namespace AliceO2
