///
/// \file CrorcChannelMaster.h
/// \author Pascal Boeschoten
///

#pragma once

#include "ChannelMaster.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
/// TODO Factor CRORC-specific things out of ChannelMaster and into this
class CrorcChannelMaster : public ChannelMaster
{
  public:

    CrorcChannelMaster(int serial, int channel, const ChannelParameters& params);
    ~CrorcChannelMaster();
    virtual void resetCard(ResetLevel::type resetLevel);
    virtual PageHandle pushNextPage();
    virtual bool isPageArrived(const PageHandle& handle);

  protected:

    virtual void deviceStartDma();
    virtual void deviceStopDma();

    /// Name for the CRORC shared data object in the shared state file
    inline static const char* crorcSharedDataName()
    {
      return "CrorcChannelMasterSharedData";
    }

  private:

    struct DataArrivalStatus
    {
        /// The status of a page's arrival
        enum type
        {
          NONE_ARRIVED = 0, // == RORC_DATA_BLOCK_NOT_ARRIVED;
          PART_ARRIVED = 1, // == RORC_NOT_END_OF_EVENT_ARRIVED
          WHOLE_ARRIVED = 2 // == RORC_LAST_BLOCK_OF_EVENT_ARRIVED
        };
    };

    /// Persistent device state/data that resides in shared memory
    class CrorcSharedData
    {
      public:
        CrorcSharedData();
        void initialize();
        InitializationState::type initializationState;
        int fifoIndexWrite; /// Index of next page available for writing
        int fifoIndexRead; /// Index of oldest non-free page
        int pageIndex; /// Index to the next free page of the DMA buffer
        long long int loopPerUsec; // Some timing parameter used during communications with the card
        double pciLoopPerUsec; // Some timing parameters used during communications with the card
        int rorcRevision;
        int diuVersion;
        int siuVersion;
    };

    /// Arm DDL function
    void armDdl(int resetMask);

    /// Enables data receiving in the RORC
    void startDataReceiving();

    /// Initializes and starts the data generator with the given parameters
    void startDataGenerator(const GeneratorParameters& gen);

    /// Pushes the initial 128 pages to the CRORC's Free FIFO
    void initializeFreeFifo(int pagesToPush);

    /// Pusha a page to the CRORC's Free FIFO
    void pushFreeFifoPage(int readyFifoIndex, void* pageBusAddress);

    /// Memory mapped data stored in the shared state file
    FileSharedObject::FileSharedObject<CrorcSharedData> crorcSharedData;

    /// Check if data has arrived
    DataArrivalStatus::type dataArrived(int index);

    /// Memory mapped file containing the readyFifo
    TypedMemoryMappedFile<ReadyFifo> mappedFileFifo;

    /// PDA DMABuffer object for the readyFifo
    PdaDmaBuffer bufferFifo;

};

} // namespace Rorc
} // namespace AliceO2
