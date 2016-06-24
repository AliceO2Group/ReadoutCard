#pragma once

#include "RorcException.h"
#include "RORC/ChannelParameters.h"
#include "FileSharedObject.h"
#include "MemoryMappedFile.h"
#include "TypedMemoryMappedFile.h"
#include "ReadyFifo.h"
#include "PdaDevice.h"
#include "PdaBar.h"
#include "PdaDmaBuffer.h"

namespace AliceO2 {
namespace Rorc {

/// Obtains a lock on the card's channel, initializes PDA objects, shared state, etc.
class ChannelMaster
{
  public:
    ChannelMaster(int serial, int channel, const ChannelParameters& params);
    ~ChannelMaster();

  private:
    /// Persistent channel state/data that resides in shared memory
    class SharedData
    {
      public:
        struct InitializationState
        {
            enum type
            {
              UNKNOWN = 0, UNINITIALIZED = 1, INITIALIZED = 2
            };
        };

        SharedData ();
        void reset(const ChannelParameters& params);
        const ChannelParameters& getParams();
        InitializationState::type getState();

      private:
        InitializationState::type initializationState;
        int fifoIndexIn;
        int fifoIndexOut;
        ChannelParameters params;
        // TODO mutex to prevent simultaneous intraprocess access?
    };

    /// Serial number of the device
    int serialNumber;

    /// DMA channel number
    int channelNumber;

    /// Memory mapped data stored in the shared state file
    FileSharedObject::LockedFileSharedObject<SharedData> sharedData;

    /// PDA device objects
    PdaDevice pdaDevice;

    /// PDA BAR object
    PdaBar pdaBar;

    /// Memory mapped file containing pages used for DMA transfer destination
    MemoryMappedFile mappedFilePages;

    /// Memory mapped file containing the readyFifo
    TypedMemoryMappedFile<ReadyFifo> mappedFileFifo;

    /// PDA DMABuffer object for the pages
    PdaDmaBuffer bufferPages;

    /// PDA DMABuffer object for the readyFifo
    PdaDmaBuffer bufferFifo;
};

} // namespace Rorc
} // namespace AliceO2
