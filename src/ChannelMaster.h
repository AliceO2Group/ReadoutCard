/// \file ChannelMaster.h
/// \brief Definition of the ChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <set>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include "ChannelParameters.h"
#include "ChannelUtilityInterface.h"
#include "InterprocessLock.h"
#include "FileSharedObject.h"
#include "MemoryMappedFile.h"
#include "PageAddress.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDmaBuffer.h"
#include "RORC/ChannelMasterInterface.h"
#include "RORC/Exception.h"
#include "RORC/Parameters.h"
#include "RorcDevice.h"
#include "TypedMemoryMappedFile.h"

//#define ALICEO2_RORC_CHANNEL_MASTER_DISABLE_LOCKGUARDS
#ifdef ALICEO2_RORC_CHANNEL_MASTER_DISABLE_LOCKGUARDS
#define CHANNELMASTER_LOCKGUARD()
#else
#define CHANNELMASTER_LOCKGUARD() LockGuard _lockGuard(getMutex());
#endif

namespace AliceO2 {
namespace Rorc {

/// Partially implements the ChannelMasterInterface. It takes care of:
/// - Interprocess synchronization
/// - PDA-based functionality that is common to the CRORC and CRU.
class ChannelMaster: public ChannelMasterInterface, public ChannelUtilityInterface
{
  public:
    using AllowedChannels = std::set<int>;

    /// Constructor for the ChannelMaster object
    /// \param cardType Type of the card
    /// \param parameters Parameters of the channel
    /// \param allowedChannels Channels allowed by this card type
    /// \param fifoSize Size of the Firmware FIFO in the DMA buffer
    ChannelMaster(CardType::type cardType, const Parameters& parameters, const AllowedChannels& allowedChannels, size_t
        fifoSize);

    ~ChannelMaster();

    virtual void startDma() final override;
    virtual void stopDma() final override;
    void resetChannel(ResetLevel::type resetLevel) final override;
    virtual uint32_t readRegister(int index) final override;
    virtual void writeRegister(int index, uint32_t value) final override;
    virtual void setLogLevel(InfoLogger::InfoLogger::Severity severity) final override;

  protected:
    using Mutex = std::mutex;
    using LockGuard = std::lock_guard<Mutex>;

    /// Template method called by startDma() to do device-specific (CRORC, RCU...) actions
    /// Note: subclasses should not call getLockGuard() in this function, as the ChannelMaster will have the lock
    /// already.
    virtual void deviceStartDma() = 0;

    /// Template method called by stopDma() to do device-specific (CRORC, RCU...) actions
    /// Note: subclasses should not call getLockGuard() in this function, as the ChannelMaster will have the lock
    /// already.
    virtual void deviceStopDma() = 0;

    /// Template method called by resetChannel() to do device-specific (CRORC, RCU...) actions
    /// Note: subclasses should not call getLockGuard() in this function, as the ChannelMaster will have the lock
    /// already.
    virtual void deviceResetChannel(ResetLevel::type resetLevel) = 0;

    /// The size of the shared state data file. It should be over-provisioned, since subclasses may also allocate their
    /// own shared data in this file.
    static size_t getSharedDataSize()
    {
      // 4KiB ought to be enough for anybody, and is also the standard x86 page size.
      // Since shared memory needs to be a multiple of page size, this should work out.
      return 4l * 1024l;
    }

    /// Name for the shared data object in the shared state file
    static std::string getSharedDataName()
    {
      return "ChannelMasterSharedData";
    }

    /// Namespace for enum describing the initialization state of the shared data
    struct InitializationState
    {
        enum type
        {
          UNKNOWN = 0, UNINITIALIZED = 1, INITIALIZED = 2
        };
    };

    /// Namespace for describing the state of the DMA
    struct DmaState
    {
        /// The state of the DMA
        enum type
        {
          UNKNOWN = 0, STOPPED = 1, STARTED = 2
        };
    };

    void* getFifoAddressBus() const
    {
      return mFifoAddressBus;
    }

    void* getFifoAddressUser() const
    {
      return mFifoAddressUser;
    }

    Mutex& getMutex()
    {
      return mMutex;
    }

    const ChannelParameters& getChannelParameters() const
    {
      return mChannelParameters;
    }

    DmaState::type getDmaState() const
    {
      return mDmaState;
    }

    int getChannelNumber() const
    {
      return mChannelNumber;
    }

    int getSerialNumber() const
    {
      return mSerialNumber;
    }

    volatile uint32_t* getBarUserspace() const
    {
      return mPdaBar->getUserspaceAddressU32();
    }

    const Pda::PdaDmaBuffer& getBufferPages() const
    {
      return *(mBufferPages.get());
    }

    const MemoryMappedFile& getMappedFilePages() const
    {
      return *(mMappedFilePages.get());
    }

    const std::vector<PageAddress>& getPageAddresses() const
    {
      return mPageAddresses;
    }

    Pda::PdaBar& getPdaBar()
    {
      return *(mPdaBar.get());
    }

    const RorcDevice& getRorcDevice() const
    {
      return *(mRorcDevice.get());
    }

    InfoLogger::InfoLogger& getLogger()
    {
      return mLogger;
    }

    InfoLogger::InfoLogger::Severity getLogLevel()
    {
      return mLogLevel;
    }

    void log(const std::string& message, boost::optional<InfoLogger::InfoLogger::Severity> severity = boost::none)
    {
      mLogger << severity.get_value_or(mLogLevel);
      mLogger << message;
      mLogger << InfoLogger::InfoLogger::endm;
    }

    /// Log a message using a callable, which is passed the logger instance
    template <class Callable>
    void withLogger(Callable callable, boost::optional<InfoLogger::InfoLogger::Severity> severity = boost::none)
    {
        mLogger << severity.get_value_or(mLogLevel);
        callable(mLogger);
        mLogger << InfoLogger::InfoLogger::endm;
    }

  private:

    /// Convert ParameterMap to ChannelParameters
    static ChannelParameters convertParameters(const Parameters& params);

    /// Validate ChannelParameters
    static void validateParameters(const ChannelParameters& ps);

    /// Helper function for partitioning the DMA buffer into FIFO and data pages
    void partitionDmaBuffer(size_t fifoSize, size_t pageSize);

    /// Check if the channel number is valid
    void checkChannelNumber(const AllowedChannels& allowedChannels);

    /// Mutex to lock thread-unsafe operations
    Mutex mMutex;

    /// Current state of the DMA
    DmaState::type mDmaState;

    /// Serial number of the device
    const int mSerialNumber;

    /// DMA channel number
    const int mChannelNumber;

    /// Lock that guards against both inter- and intra-process ownership
    boost::scoped_ptr<Interprocess::Lock> mInterprocessLock;

    /// PDA device objects
    boost::scoped_ptr<RorcDevice> mRorcDevice;

    /// PDA BAR object
    boost::scoped_ptr<Pda::PdaBar> mPdaBar;

    /// Memory mapped file containing pages used for DMA transfer destination
    boost::scoped_ptr<MemoryMappedFile> mMappedFilePages;

    /// PDA DMABuffer object for the pages
    boost::scoped_ptr<Pda::PdaDmaBuffer> mBufferPages;

    /// Addresses to pages in the DMA buffer
    std::vector<PageAddress> mPageAddresses;

    /// Userspace address of FIFO in DMA buffer
    void* mFifoAddressUser;

    /// Bus address of FIFO in DMA buffer
    void* mFifoAddressBus;

    /// Parameters of this channel TODO refactor
    ChannelParameters mChannelParameters;

    /// InfoLogger instance
    InfoLogger::InfoLogger mLogger;

    /// Current log level
    InfoLogger::InfoLogger::Severity mLogLevel;
};

} // namespace Rorc
} // namespace AliceO2
