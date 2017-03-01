/// \file ChannelMasterBase.h
/// \brief Definition of the ChannelMasterBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <set>
#include <vector>
#include <mutex>
#include <boost/scoped_ptr.hpp>
#include "ChannelBase.h"
#include "ChannelParameters.h"
#include "ChannelPaths.h"
#include "ChannelUtilityInterface.h"
#include "InterprocessLock.h"
#include "RORC/ChannelMasterInterface.h"
#include "RORC/Exception.h"
#include "RORC/Parameters.h"
#include "RORC/MemoryMappedFile.h"
#include "Utilities/Util.h"

//#define ALICEO2_RORC_CHANNEL_MASTER_DISABLE_LOCKGUARDS
#ifdef ALICEO2_RORC_CHANNEL_MASTER_DISABLE_LOCKGUARDS
#define CHANNELMASTER_LOCKGUARD()
#else
#define CHANNELMASTER_LOCKGUARD() LockGuard _lockGuard(getMutex());
#endif

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
    /// \param cardType Type of the card
    /// \param parameters Parameters of the channel
    /// \param serialNumber Serial number of the card
    /// \param allowedChannels Channels allowed by this card type
    ChannelMasterBase(CardType::type cardType, const Parameters& parameters, int serialNumber,
        const AllowedChannels& allowedChannels);

    virtual ~ChannelMasterBase();

    virtual void pushSuperpage(size_t, size_t) override
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

    virtual SuperpageStatus getSuperpageStatus() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual SuperpageStatus popSuperpage() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

    virtual void fillSuperpages() override
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("not yet implemented"));
    }

  protected:
    using Mutex = std::mutex;
    using LockGuard = std::lock_guard<Mutex>;

    class BufferProvider
    {
      public:
        virtual ~BufferProvider()
        {
        }

        void* getBufferStartAddress() const
        {
          return reservedStartAddress;
        }

        size_t getBufferSize() const
        {
          return bufferSize;
        }

        size_t getReservedOffset() const
        {
          return reservedOffset;
        }

        void* getReservedStartAddress() const
        {
          return reservedStartAddress;
        }

        size_t getReservedSize() const
        {
          return reservedSize;
        }

        size_t getDmaOffset() const
        {
          return dmaOffset;
        }

        void* getDmaStartAddress() const
        {
          return dmaStartAddress;
        }

        size_t getDmaSize() const
        {
          return dmaSize;
        }

      protected:
        void* bufferStartAddress;
        size_t bufferSize;
        void* reservedStartAddress;
        size_t reservedSize;
        size_t reservedOffset;
        void* dmaStartAddress;
        size_t dmaSize;
        size_t dmaOffset;
    };

    /// Namespace for enum describing the initialization state of the shared data
    struct InitializationState
    {
        enum type
        {
          UNKNOWN = 0, UNINITIALIZED = 1, INITIALIZED = 2
        };
    };

    Mutex& getMutex()
    {
      return mMutex;
    }

    const ChannelParameters& getChannelParameters() const
    {
      return mChannelParameters;
    }

    int getChannelNumber() const
    {
      return mChannelNumber;
    }

    int getSerialNumber() const
    {
      return mSerialNumber;
    }

    virtual void setLogLevel(InfoLogger::InfoLogger::Severity severity) final override
    {
      ChannelBase::setLogLevel(severity);
    }

    void log(const std::string& message, boost::optional<InfoLogger::InfoLogger::Severity> severity = boost::none)
    {
      ChannelBase::log(mSerialNumber, mChannelNumber, message, severity);
    }

    ChannelPaths getPaths()
    {
      return {mCardType, mSerialNumber, mChannelNumber};
    }

    const BufferProvider& getBufferProvider()
    {
      return *mBufferProvider;
    }

  private:

    class BufferProviderMemory : public BufferProvider
    {
      public:
        BufferProviderMemory(const BufferParameters::Memory& parameters)
        {
          bufferStartAddress = parameters.bufferStart;
          bufferSize = parameters.bufferSize;
          reservedOffset = Utilities::pointerDiff(parameters.reservedStart, parameters.bufferStart);
          reservedStartAddress = parameters.reservedStart;
          reservedSize = parameters.reservedSize;
          dmaOffset = Utilities::pointerDiff(parameters.dmaStart, parameters.bufferStart);
          dmaStartAddress = parameters.dmaStart;
          dmaSize = parameters.dmaSize;
        }

        virtual ~BufferProviderMemory()
        {
        }
    };

    class BufferProviderFile : public BufferProvider
    {
      public:
        BufferProviderFile(const BufferParameters::File& parameters)
        {
          Utilities::resetSmartPtr(mMappedFilePages, parameters.path, parameters.size);
          bufferStartAddress = mMappedFilePages->getAddress();
          bufferSize = parameters.size;
          reservedOffset = parameters.reservedStart;
          reservedStartAddress = Utilities::offsetBytes(bufferStartAddress, parameters.reservedStart);
          reservedSize = parameters.reservedSize;
          dmaOffset = parameters.dmaStart;
          dmaStartAddress = Utilities::offsetBytes(bufferStartAddress, parameters.dmaStart);
          dmaSize = parameters.dmaSize;
        }

        virtual ~BufferProviderFile()
        {
        }

      private:
        /// Memory mapped file containing pages used for DMA transfer destination
        boost::scoped_ptr<MemoryMappedFile> mMappedFilePages;
    };

    /// Convert ParameterMap to ChannelParameters
    static ChannelParameters convertParameters(const Parameters& params);

    /// Validate ChannelParameters
    static void validateParameters(const ChannelParameters& ps);

    /// Check if the channel number is valid
    void checkChannelNumber(const AllowedChannels& allowedChannels);

    /// Mutex to lock thread-unsafe operations
    Mutex mMutex;

    /// Type of the card
    CardType::type mCardType;

    /// Serial number of the device
    int mSerialNumber;

    /// DMA channel number
    const int mChannelNumber;

    /// Lock that guards against both inter- and intra-process ownership
    boost::scoped_ptr<Interprocess::Lock> mInterprocessLock;

    /// Parameters of this channel TODO refactor
    ChannelParameters mChannelParameters;

    /// Contains addresses & size of the buffer
    std::unique_ptr<BufferProvider> mBufferProvider;
};

} // namespace Rorc
} // namespace AliceO2
