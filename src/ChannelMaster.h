///
/// \file ChannelMaster.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <vector>
#include <boost/scoped_ptr.hpp>
#include "RORC/Parameters.h"
#include "RORC/ChannelMasterInterface.h"
#include "RORC/Exception.h"
#include "FileSharedObject.h"
#include "MemoryMappedFile.h"
#include "TypedMemoryMappedFile.h"
#include "RorcDevice.h"
#include "ChannelParameters.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDmaBuffer.h"

#include "ChannelUtilityInterface.h"

namespace AliceO2 {
namespace Rorc {

/// Partially implements the ChannelMasterInterface. It takes care of:
/// - Interprocess synchronization
/// - PDA-based functionality that is common to the CRORC and CRU.
class ChannelMaster: public ChannelMasterInterface, public ChannelUtilityInterface
{
  public:

    /// Constructor for the ChannelMaster object
    /// \param cardType Type of the card
    /// \param serial Serial number of the card
    /// \param channel Channel number of the channel
    /// \param params Parameters of the channel
    /// \param additionalBuffers Subclasses must provide here the amount of DMA buffers the channel uses in total,
    ///        excluding the one used by the ChannelMaster itself.
    ChannelMaster(CardType::type cardType, int serial, int channel, const Parameters::Map& params,
        int additionalBuffers);

    /// Constructor for the ChannelMaster object, without passing ChannelParameters
    /// Construction will not succeed without these criteria:
    ///  - A ChannelMaster *with* ChannelParameters was already successfully constructed
    ///  - It successfully released
    ///  - Its state has been preserved in shared memory
    /// \param cardType Type of the card
    /// \param serial Serial number of the card
    /// \param channel Channel number of the channel
    /// \param additionalBuffers Subclasses must provide here the amount of DMA buffers the channel uses in total,
    ///        excluding the one used by the ChannelMaster itself.
    ChannelMaster(CardType::type cardType, int serial, int channel, int additionalBuffers);

    ~ChannelMaster();

    virtual void startDma() final override;
    virtual void stopDma() final override;
    virtual uint32_t readRegister(int index) final override;
    virtual void writeRegister(int index, uint32_t value) final override;

    /// Convert ParameterMap to ChannelParameters
    static ChannelParameters convertParameters(const Parameters::Map& params);
    static void validateParameters(const ChannelParameters& ps);

  protected:

    /// Get the buffer ID belonging to the buffer. Note that index 0 is reserved for use by ChannelMaster
    /// See dmaBuffersPerChannel() for more info
    /// Non-virtual because it must be callable in constructors, and there's no need to override it.
    int getBufferId(int index) const;

    /// Template method called by startDma() to do device-specific (CRORC, RCU...) actions
    virtual void deviceStartDma() = 0;

    /// Template method called by stopDma() to do device-specific (CRORC, RCU...) actions
    virtual void deviceStopDma() = 0;

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

    /// A simple struct that holds the userspace and bus address of a page
    struct PageAddress
    {
        void* user;
        void* bus;
    };

  private:

    /// Persistent channel state/data that resides in shared memory
    class SharedData
    {
      public:
        SharedData();

        void initialize(const ChannelParameters& params);

        const ChannelParameters& getParams() const
        {
          return params;
        }

        InitializationState::type getState() const
        {
          return initializationState;
        }

        DmaState::type dmaState;
        InitializationState::type initializationState;

      private:
        ChannelParameters params;
    };

    /// Card type ofthe device
    const CardType::type cardType;

    /// Serial number of the device
    const int serialNumber;

    /// DMA channel number
    const int channelNumber;

    /// Amount of DMA buffers per channel that will be registered to PDA
    /// Is the sum of the buffers needed by this class and by the subclass. The subclass indicates its need in the
    /// constructor of this class.
    const int dmaBuffersPerChannel;

    /// Memory mapped data stored in the shared state file
    boost::scoped_ptr<FileSharedObject::LockedFileSharedObject<SharedData>> sharedData;

    /// Mutex to guard against both interprocess and intraprocess simultaneous access
    boost::scoped_ptr<boost::interprocess::named_mutex> interProcessMutex;

    /// Lock guard for interprocess_mutex
    boost::scoped_ptr<FileSharedObject::ThrowingLockGuard<boost::interprocess::named_mutex>> mutexGuard;

    /// PDA device objects
    boost::scoped_ptr<RorcDevice> rorcDevice;

    /// PDA BAR object
    boost::scoped_ptr<Pda::PdaBar> pdaBar;

    /// Memory mapped file containing pages used for DMA transfer destination
    boost::scoped_ptr<MemoryMappedFile> mappedFilePages;

    /// PDA DMABuffer object for the pages
    boost::scoped_ptr<Pda::PdaDmaBuffer> bufferPages;

    /// Addresses to pages in the DMA buffer
    std::vector<PageAddress> pageAddresses;

    void constructorCommonPhaseOne();
    void constructorCommonPhaseTwo();

  public:

    SharedData& getSharedData() const
    {
      return *(sharedData->get());
    }

    const ChannelParameters& getParams() const
    {
      return sharedData->get()->getParams();
    }

    int getChannelNumber() const
    {
      return channelNumber;
    }

    int getSerialNumber() const
    {
      return serialNumber;
    }

    volatile uint32_t* getBarUserspace() const
    {
      return pdaBar->getUserspaceAddressU32();;
    }

    const Pda::PdaDmaBuffer& getBufferPages() const
    {
      return *(bufferPages.get());
    }

    const MemoryMappedFile& getMappedFilePages() const
    {
      return *(mappedFilePages.get());
    }

    std::vector<PageAddress>& getPageAddresses()
    {
      return pageAddresses;
    }

    const Pda::PdaBar& getPdaBar() const
    {
      return *(pdaBar.get());
    }

    const RorcDevice& getRorcDevice() const
    {
      return *(rorcDevice.get());
    }

};

} // namespace Rorc
} // namespace AliceO2
