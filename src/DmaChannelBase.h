/// \file DmaChannelBase.h
/// \brief Definition of the DmaChannelBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <set>
#include <vector>
#include <memory>
#include <mutex>
#include <boost/optional.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include "BufferProvider.h"
#include "CardDescriptor.h"
#include "ChannelPaths.h"
#include "InterprocessLock.h"
#include "ReadoutCard/DmaChannelInterface.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/Parameters.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {

/// Partially implements the DmaChannelInterface. It provides:
/// - Interprocess synchronization
/// - Creation of files and directories related to the channel
/// - Logging facilities
class DmaChannelBase: public DmaChannelInterface
{
  public:
    using AllowedChannels = std::set<int>;

    /// Constructor for the DmaChannel object
    /// \param cardDescriptor Card descriptor
    /// \param parameters Parameters of the channel
    /// \param allowedChannels Channels allowed by this card type
    DmaChannelBase(CardDescriptor cardDescriptor, const Parameters& parameters,
        const AllowedChannels& allowedChannels);
    virtual ~DmaChannelBase();

    /// Default implementation for optional function
    virtual boost::optional<float> getTemperature() override
    {
      return {};
    }

    /// Default implementation for optional function
    virtual boost::optional<std::string> getFirmwareInfo() override
    {
      return {};
    }

  protected:
    /// Namespace for enum describing the initialization state of the shared data
    struct InitializationState
    {
        enum type
        {
          UNKNOWN = 0, UNINITIALIZED = 1, INITIALIZED = 2
        };
    };

    int getChannelNumber() const
    {
      return mChannelNumber;
    }

    int getSerialNumber() const
    {
      return mCardDescriptor.serialNumber;
    }

    const CardDescriptor& getCardDescriptor() const
    {
      return mCardDescriptor;
    }

    ChannelPaths getPaths()
    {
      return {getCardDescriptor().pciAddress, getChannelNumber()};
    }

    const BufferProvider& getBufferProvider()
    {
      return *mBufferProvider;
    }

    void log(const std::string& message, boost::optional<InfoLogger::InfoLogger::Severity> severity = boost::none);

    InfoLogger::InfoLogger& getLogger()
    {
      return mLogger;
    }

    InfoLogger::InfoLogger::Severity getLogLevel()
    {
      return mLogLevel;
    }

    virtual void setLogLevel(InfoLogger::InfoLogger::Severity severity) final override
    {
      mLogLevel = severity;
    }

  private:
    /// Check if the channel number is valid
    void checkChannelNumber(const AllowedChannels& allowedChannels);

    /// Type of the card
    const CardDescriptor mCardDescriptor;

    /// DMA channel number
    const int mChannelNumber;

    /// Lock that guards against both inter- and intra-process ownership
    std::unique_ptr<Interprocess::Lock> mInterprocessLock;

    /// Contains addresses & size of the buffer
    std::unique_ptr<BufferProvider> mBufferProvider;

    /// InfoLogger instance
    InfoLogger::InfoLogger mLogger;

    /// Current log level
    InfoLogger::InfoLogger::Severity mLogLevel;
};

} // namespace roc
} // namespace AliceO2
