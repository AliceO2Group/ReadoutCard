#pragma once

#include <vector>
#include "RorcException.h"
#include "RORC/ChannelParameters.h"
#include "FileSharedObject.h"
#include "MemoryMappedFile.h"
#include "TypedMemoryMappedFile.h"
#include "ReadyFifo.h"
#include "PdaDevice.h"
#include "PdaBar.h"
#include "PdaDmaBuffer.h"
#include "RORC/ChannelMasterInterface.h"

namespace AliceO2 {
namespace Rorc {

/// Obtains a lock on the card's channel, initializes PDA objects, shared state, etc.
class ChannelMaster: public ChannelMasterInterface
{
  public:
    ChannelMaster(int serial, int channel, const ChannelParameters& params);
    ~ChannelMaster();

    virtual void startDma();
    virtual void stopDma();
    virtual void resetCard(ResetLevel::type resetLevel);
    virtual uint32_t readRegister(int index);
    virtual void writeRegister(int index, uint32_t value);
    virtual PageHandle pushNextPage();
    virtual bool isPageArrived(const PageHandle& handle);
    virtual Page getPage(const PageHandle& handle);
    virtual void markPageAsRead(const PageHandle& handle);

  private:
    struct InitializationState
    {
        enum type
        {
          UNKNOWN = 0, UNINITIALIZED = 1, INITIALIZED = 2
        };
    };

    struct DmaState
    {
        enum type
        {
          UNKNOWN = 0, STOPPED = 1, STARTED = 2
        };
    };

    struct DataArrivalStatus
    {
        enum type
        {
          NONE_ARRIVED = 0, // == RORC_DATA_BLOCK_NOT_ARRIVED;
          PART_ARRIVED = 1, // == RORC_NOT_END_OF_EVENT_ARRIVED
          WHOLE_ARRIVED = 2 // == RORC_LAST_BLOCK_OF_EVENT_ARRIVED
        };
    };

    /// Persistent channel state/data that resides in shared memory
    class SharedData
    {
      public:
        SharedData();
        void reset(const ChannelParameters& params);
        const ChannelParameters& getParams();
        InitializationState::type getState();

        int fifoIndexWrite; /// Index of next page available for writing
        int fifoIndexRead; /// Index of oldest non-free page
        int pageIndex; /// Index to the next free page of the DMA buffer
        long long int loopPerUsec; // Some timing parameter used during communications with the card
        double pciLoopPerUsec; // Some timing parameters used during communications with the card
        DmaState::type dmaState;
        InitializationState::type initializationState;

      private:
        ChannelParameters params;
        // TODO mutex to prevent simultaneous intraprocess access?
    };

    const ChannelParameters& getParams();
    void armDdl(int resetMask);
    void startDataReceiving();
    void armDataGenerator(const GeneratorParameters& gen);
    void startDataGenerator(int maxEvents);
    void initializeFreeFifo(int pagesToPush);
    void pushFreeFifoPage(int readyFifoIndex, void* pageBusAddress);
    DataArrivalStatus::type dataArrived(int index);

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

    /// Addresses to pages in the DMA buffer
    struct PageAddress
    {
        void* user;
        void* bus;
    };
    std::vector<PageAddress> pageAddresses;

    /// Array to keep track of read pages (false: wasn't read out, true: was read out).
    std::vector<bool> pageWasReadOut;
};

} // namespace Rorc
} // namespace AliceO2
