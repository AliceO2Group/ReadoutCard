///
/// \file ChannelMaster.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <memory>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "RorcException.h"
#include "RORC/ChannelParameters.h"
#include "FileSharedObject.h"
#include "MemoryMappedFile.h"
#include "TypedMemoryMappedFile.h"
#include "RorcDevice.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDmaBuffer.h"
#include "RORC/ChannelMasterInterface.h"
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
    /// \param serial Serial number of the card
    /// \param channel Channel number of the channel
    /// \param params Parameters of the channel
    /// \param additionalBuffers Subclasses must provide here the amount of DMA buffers the channel uses in total,
    ///        excluding the one used by the ChannelMaster itself.
    ChannelMaster(int serial, int channel, const ChannelParameters& params, int additionalBuffers);

    /// Constructor for the ChannelMaster object, without passing ChannelParameters
    /// Construction will not succeed without these criteria:
    ///  - A ChannelMaster *with* ChannelParameters was already successfully constructed
    ///  - It successfully released
    ///  - Its state has been preserved in shared memory
    /// \param serial Serial number of the card
    /// \param channel Channel number of the channel
    /// \param additionalBuffers Subclasses must provide here the amount of DMA buffers the channel uses in total,
    ///        excluding the one used by the ChannelMaster itself.
    ChannelMaster(int serial, int channel, int additionalBuffers);

    ~ChannelMaster();

    virtual void startDma() override;
    virtual void stopDma() override;
    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;

    static void validateParameters(const ChannelParameters& params);

  protected:

    /// Get the buffer ID belonging to the buffer. Note that index 0 is reserved for use by ChannelMaster
    /// See dmaBuffersPerChannel() for more info
    int getBufferId(int index);

    /// Template method called by startDma() to do device-specific (CRORC, RCU...) actions
    virtual void deviceStartDma() = 0;

    /// Template method called by stopDma() to do device-specific (CRORC, RCU...) actions
    virtual void deviceStopDma() = 0;

    /// The size of the shared state data file. It should be over-provisioned, since subclasses may also allocate their
    /// own shared data in this file.
    inline static size_t sharedDataSize()
    {
      return 4l * 1024l; // 4k ought to be enough for anybody
    }

    /// Name for the shared data object in the shared state file
    inline static const char* sharedDataName()
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

    /// Persistent channel state/data that resides in shared memory
    class SharedData
    {
      public:
        SharedData();

        void initialize(const ChannelParameters& params);
        const ChannelParameters& getParams();
        InitializationState::type getState();

        DmaState::type dmaState;
        InitializationState::type initializationState;

      private:
        ChannelParameters params;
        // TODO mutex to prevent simultaneous intraprocess access?
    };

    /// A simple struct that holds the userspace and bus address of a page
    struct PageAddress
    {
        void* user;
        void* bus;
    };

    /// Getter for parameters in SharedData
    const ChannelParameters& getParams();

    /// Serial number of the device
    int serialNumber;

    /// DMA channel number
    int channelNumber;

    /// Amount of DMA buffers per channel that will be registered to PDA
    int dmaBuffersPerChannel;

    /// Memory mapped data stored in the shared state file
    boost::scoped_ptr<FileSharedObject::LockedFileSharedObject<SharedData>> sharedData;

    /// PDA device objects
    boost::scoped_ptr<RorcDevice> rorcDevice;

    /// PDA BAR object
    boost::scoped_ptr<PdaBar> pdaBar;

    /// BAR userspace address
    uint32_t volatile * barUserspace;

    /// Memory mapped file containing pages used for DMA transfer destination
    boost::scoped_ptr<MemoryMappedFile> mappedFilePages;

    /// PDA DMABuffer object for the pages
    boost::scoped_ptr<PdaDmaBuffer> bufferPages;

    /// Addresses to pages in the DMA buffer
    std::vector<PageAddress> pageAddresses;

  private:

    void constructorCommonPhaseOne();
    void constructorCommonPhaseTwo();
};

} // namespace Rorc
} // namespace AliceO2
