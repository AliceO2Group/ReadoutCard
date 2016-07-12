///
/// \file CrorcChannelMaster.h
/// \author Pascal Boeschoten
///

#pragma once

#include "ChannelMaster.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
class CrorcChannelMaster : public ChannelMaster
{
  public:

    CrorcChannelMaster(int serial, int channel, const ChannelParameters& params);
    ~CrorcChannelMaster();
    virtual void resetCard(ResetLevel::type resetLevel);
    virtual PageHandle pushNextPage();
    virtual bool isPageArrived(const PageHandle& handle);
    virtual Page getPage(const PageHandle& handle);
    virtual void markPageAsRead(const PageHandle& handle);

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
        int fifoIndexWrite; ///< Index of next FIFO page available for writing
        int fifoIndexRead; ///< Index of oldest non-free FIFO page
        int bufferPageIndex; ///< Index of next DMA buffer page available for writing
        long long int loopPerUsec; ///< Some timing parameter used during communications with the card
        double pciLoopPerUsec; ///< Some timing parameters used during communications with the card
        int rorcRevision;
        int siuVersion;
        int diuVersion;
    };

    /// Enables data receiving in the RORC
    void startDataReceiving();

    /// Initializes and starts the data generator with the given parameters
    /// \param generatorParameters The parameters for the data generator.
    void startDataGenerator(const GeneratorParameters& generatorParameters);

    /// Pushes the initial 128 pages to the CRORC's Free FIFO
    void initializeFreeFifo();

    /// Pushes a page to the CRORC's Free FIFO
    /// \param readyFifoIndex Index of the Ready FIFO to write the page's transfer status to
    /// \param pageBusAddress Address on the bus to push the page to
    void pushFreeFifoPage(int readyFifoIndex, void* pageBusAddress);

    /// Get the bus address of the Ready FIFO
    void* getReadyFifoBusAddress();

    /// Get the userspace Ready FIFO object
    ReadyFifo& getReadyFifo();

    /// Check if data has arrived
    DataArrivalStatus::type dataArrived(int index);

    /// Arms C-RORC data generator
    void crorcArmDataGenerator();

    /// Arms DDL
    /// \param resetMask The reset mask. See the RORC_RESET_* macros in rorc.h
    void crorcArmDdl(int resetMask);

    /// Find and store DIU version
    void crorcInitDiuVersion();

    /// Checks if link is up
    void crorcCheckLink();

    /// Send a command to the SIU
    /// \param command The command to send to the SIU. These are probably the macros under 'interface commands' in
    ///   the header ddl_def.h
    void crorcSiuCommand(int command);

    /// Send a command to the DIU
    /// \param command The command to send to the SIU. These are probably the macros under 'interface commands' in
    ///   the header ddl_def.h
    void crorcDiuCommand(int command);

    /// Reset the C-RORC
    void crorcReset();

    /// Checks if the C-RORC's Free FIFO is empty
    void crorcCheckFreeFifoEmpty();

    /// Starts data receiving
    void crorcStartDataReceiver();

    /// Starts the trigger
    void crorcStartTrigger();

    /// Stops the trigger
    void crorcStopTrigger();

    /// Set SIU loopback
    void crorcSetSiuLoopback();

    /// Memory mapped file containing the readyFifo
    TypedMemoryMappedFile<ReadyFifo> mappedFileFifo;

    /// PDA DMABuffer object for the Ready FIFO
    PdaDmaBuffer bufferReadyFifo;

    /// Memory mapped data stored in the shared state file
    FileSharedObject::FileSharedObject<CrorcSharedData> crorcSharedData;

    /// Mapping from fifo page index to DMA buffer index
    std::vector<int> bufferPageIndexes;

    /// Array to keep track of read pages (false: wasn't read out, true: was read out).
    std::vector<bool> pageWasReadOut;
};

} // namespace Rorc
} // namespace AliceO2
