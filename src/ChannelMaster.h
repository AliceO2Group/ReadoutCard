/// \file ChannelMaster.h
/// \brief Definition of the ChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <set>
#include <vector>
#include <boost/scoped_ptr.hpp>
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
#define CHANNELMASTER_LOCKGUARD() auto&& _lockGuard = getLockGuard()
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
    /// \param serial Serial number of the card
    /// \param channel Channel number of the channel
    /// \param params Parameters of the channel
    /// \param additionalBuffers Subclasses must provide here the amount of DMA buffers the channel uses in total,
    ///        excluding the one used by the ChannelMaster itself.
    /// \param allowedChannels Channels allowed by this card type
    ChannelMaster(CardType::type cardType, int serial, int channel, const Parameters::Map& parameterMap,
        int additionalBuffers, const AllowedChannels& allowedChannels);

    ~ChannelMaster();

    virtual void startDma() final override;
    virtual void stopDma() final override;
    void resetChannel(ResetLevel::type resetLevel) final override;
    virtual uint32_t readRegister(int index) final override;
    virtual void writeRegister(int index, uint32_t value) final override;

    /// Convert ParameterMap to ChannelParameters
    static ChannelParameters convertParameters(const Parameters::Map& params);
    static void validateParameters(const ChannelParameters& ps);

  protected:

    using Mutex = std::mutex;
    using LockGuard = std::lock_guard<Mutex>;

    /// Get the buffer ID belonging to the buffer. Note that index 0 is reserved for use by ChannelMaster
    /// See dmaBuffersPerChannel() for more info
    /// Non-virtual because it must be callable in constructors, and there's no need to override it.
    int getBufferId(int index) const;

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

    /// Acquires a mutex lock to make operations thread-safe
    /// usage:
    /// auto&& lockGuard = getLockGuard();
    LockGuard getLockGuard()
    {
      mMutex.lock();
      return { mMutex, std::adopt_lock };
    }

  private:

    void checkChannelNumber(const AllowedChannels& allowedChannels);

    DmaState::type mDmaState;

    /// Card type ofthe device
    const CardType::type mCardType;

    /// Serial number of the device
    const int mSerialNumber;

    /// DMA channel number
    const int mChannelNumber;

    /// Amount of DMA buffers per channel that will be registered to PDA
    /// Is the sum of the buffers needed by this class and by the subclass. The subclass indicates its need in the
    /// constructor of this class.
    const int dmaBuffersPerChannel;

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

    ChannelParameters mChannelParameters;

    /// Mutex to lock operations
    std::mutex mMutex;

  public:

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

    std::vector<PageAddress>& getPageAddresses()
    {
      return mPageAddresses;
    }

    const Pda::PdaBar& getPdaBar() const
    {
      return *(mPdaBar.get());
    }

    const RorcDevice& getRorcDevice() const
    {
      return *(mRorcDevice.get());
    }
};

} // namespace Rorc
} // namespace AliceO2
