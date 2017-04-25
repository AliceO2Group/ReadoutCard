/// \file ChannelMasterBase.h
/// \brief Definition of the ChannelMasterBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <set>
#include <vector>
#include <memory>
#include <mutex>
#include "BufferProvider.h"
#include "CardDescriptor.h"
#include "ChannelBase.h"
#include "ChannelPaths.h"
#include "ChannelUtilityInterface.h"
#include "InterprocessLock.h"
#include "RORC/ChannelMasterInterface.h"
#include "RORC/Exception.h"
#include "RORC/Parameters.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace Rorc {

/// Partially implements the ChannelMasterInterface. It provides:
/// - Interprocess synchronization
/// - Creation of files and directories related to the channel
class ChannelMasterBase: public ChannelBase, public ChannelMasterInterface, public ChannelUtilityInterface
{
  public:
    using AllowedChannels = std::set<int>;

    /// Constructor for the ChannelMaster object
    /// \param cardDescriptor Card descriptor
    /// \param parameters Parameters of the channel
    /// \param allowedChannels Channels allowed by this card type
    ChannelMasterBase(CardDescriptor cardDescriptor, const Parameters& parameters,
        const AllowedChannels& allowedChannels);

    virtual ~ChannelMasterBase();

    virtual void pushSuperpage(Superpage) override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual int getSuperpageQueueCount() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual int getSuperpageQueueAvailable() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual int getSuperpageQueueCapacity() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual Superpage getSuperpage() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual Superpage popSuperpage() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual void fillSuperpages() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual boost::optional<float> getTemperature() override
    {
      return {};
    }

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

    virtual void setLogLevel(InfoLogger::InfoLogger::Severity severity) final override
    {
      ChannelBase::setLogLevel(severity);
    }

    void log(const std::string& message, boost::optional<InfoLogger::InfoLogger::Severity> severity = boost::none)
    {
      ChannelBase::log(getSerialNumber(), getChannelNumber(), message, severity);
    }

    ChannelPaths getPaths()
    {
      return {getCardDescriptor().pciAddress, getChannelNumber()};
    }

    const BufferProvider& getBufferProvider()
    {
      return *mBufferProvider;
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
};

} // namespace Rorc
} // namespace AliceO2
