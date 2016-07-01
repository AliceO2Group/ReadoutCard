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
    inline static const char* crorcSharedDataName()
    {
      return "CrorcChannelMasterSharedData";
    }

  private:

    /// Persistent device state/data that resides in shared memory
    class CrorcSharedData
    {
      public:
        CrorcSharedData();
        void reset();
        int fifoIndexWrite; /// Index of next page available for writing
        int fifoIndexRead; /// Index of oldest non-free page
        int pageIndex; /// Index to the next free page of the DMA buffer
        long long int loopPerUsec; // Some timing parameter used during communications with the card
        double pciLoopPerUsec; // Some timing parameters used during communications with the card
        InitializationState::type initializationState;
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
