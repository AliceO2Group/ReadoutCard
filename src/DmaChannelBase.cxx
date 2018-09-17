/// \file DmaChannelBase.cxx
/// \brief Implementation of the DmaChannelBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <boost/filesystem.hpp>
#include "DmaChannelBase.h"
#include <iostream>
#include "ChannelPaths.h"
#include "Common/System.h"
#include "Utilities/SmartPointer.h"
#include "Visitor.h"

namespace AliceO2 {
namespace roc {
namespace {
void checkParameters(const Parameters& parameters)
{
  // Generator enabled is not allowed in conjunction with none loopback mode
  auto enabled = parameters.getGeneratorEnabled();
  auto loopback = parameters.getGeneratorLoopback();
  if (enabled && loopback && (*enabled && (*loopback == LoopbackMode::None))) {
    BOOST_THROW_EXCEPTION(
        InvalidParameterException() << ErrorInfo::Message("Generator enabled but 'None' loopback mode specified"));
  }
}
} // Anonymous namespace

namespace b = boost;
namespace bfs = boost::filesystem;

void DmaChannelBase::checkChannelNumber(const AllowedChannels& allowedChannels)
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

DmaChannelBase::DmaChannelBase(CardDescriptor cardDescriptor, const Parameters& parameters,
    const AllowedChannels& allowedChannels)
    : mCardDescriptor(cardDescriptor), mChannelNumber(parameters.getChannelNumberRequired())
{
#ifndef NDEBUG
  log("Backend compiled without NDEBUG; performance may be severely degraded", InfoLogger::InfoLogger::Info);
#endif

  // Check the channel number is allowed
  checkChannelNumber(allowedChannels);

  // Do some basic Parameters validity checks
  checkParameters(parameters);

  // Create parent directories
  auto paths = getPaths();
  for (const auto& p : {paths.fifo(), paths.lock()}) {
    Common::System::makeParentDirectories(p);
  }

  // Get lock
  log("Acquiring DMA channel lock", InfoLogger::InfoLogger::Debug);
  try {
    Utilities::resetSmartPtr(mInterprocessLock, paths.lock(), paths.namedMutex());
  }
  catch (const NamedMutexLockException& exception) {
    if (parameters.getForcedUnlockEnabled().get_value_or(false)) {
      log("Failed to acquire DMA channel mutex. Forced unlock enabled - attempting cleanup and retry",
          InfoLogger::InfoLogger::Warning);
      // The user has indicated to the driver that it is safe to force the unlock. We do it by deleting the lock.
      boost::interprocess::named_mutex::remove(paths.namedMutex().c_str());
      // Try again...
      Utilities::resetSmartPtr(mInterprocessLock, paths.lock(), paths.namedMutex());
    } else {
      log("Failed to acquire DMA channel lock", InfoLogger::InfoLogger::Debug);
      throw;
    }
  }
  log("Acquired DMA channel lock", InfoLogger::InfoLogger::Debug);
}

DmaChannelBase::~DmaChannelBase()
{
  log("Releasing DMA channel lock", InfoLogger::InfoLogger::Debug);

  std::string channelLockPath = "/dev/shm/AliceO2_RoC_" + std::string(getCardDescriptor().pciAddress.toString()) + "_Channel_" + std::to_string((int) getChannelNumber ()) + ".lock";
  std::string channelMutexPath = "/dev/shm/sem.AliceO2_RoC_" + std::string(getCardDescriptor().pciAddress.toString()) + "_Channel_" + std::to_string((int) getChannelNumber ()) + "_Mutex";
  bfs::remove(channelLockPath);
  bfs::remove(channelMutexPath);
}

void DmaChannelBase::log(const std::string& message, boost::optional<InfoLogger::InfoLogger::Severity> severity)
{
  mLogger << severity.get_value_or(mLogLevel);
  mLogger << "[pci=" << getCardDescriptor().pciAddress.toString();
  if (auto serial = getSerialNumber()) {
    mLogger << " serial=" << serial.get();
  }
  mLogger << " channel=" << getChannelNumber() << "] ";
  mLogger << message;
  mLogger << InfoLogger::InfoLogger::endm;
}


} // namespace roc
} // namespace AliceO2
