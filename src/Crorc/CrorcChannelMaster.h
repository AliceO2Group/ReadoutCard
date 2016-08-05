///
/// \file CrorcChannelMaster.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <memory>
#include "ChannelMaster.h"
#include "ReadyFifo.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
class CrorcChannelMaster : public ChannelMaster
{
  public:

    CrorcChannelMaster(int serial, int channel);
    CrorcChannelMaster(int serial, int channel, const ChannelParameters& params);
    virtual ~CrorcChannelMaster() override;

    virtual void resetCard(ResetLevel::type resetLevel) override;
    virtual PageHandle pushNextPage() override;
    virtual bool isPageArrived(const PageHandle& handle) override;
    virtual Page getPage(const PageHandle& handle) override;
    virtual void markPageAsRead(const PageHandle& handle) override;
    virtual CardType::type getCardType() override;

    virtual std::vector<uint32_t> utilityCopyFifo() override;
    virtual void utilityPrintFifo(std::ostream& os) override;
    virtual void utilitySetLedState(bool state) override;
    virtual void utilitySanityCheck(std::ostream& os) override;
    virtual void utilityCleanupState() override;
    virtual int utilityGetFirmwareVersion() override;

  protected:

    virtual void deviceStartDma() override;
    virtual void deviceStopDma() override;

    /// Name for the CRORC shared data object in the shared state file
    inline static const char* crorcSharedDataName()
    {
      return "CrorcChannelMasterSharedData";
    }

  private:

    /// Namespace for enum describing the status of a page's arrival
    struct DataArrivalStatus
    {
        enum type
        {
          NONE_ARRIVED,
          PART_ARRIVED,
          WHOLE_ARRIVED,
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
    boost::scoped_ptr<TypedMemoryMappedFile<ReadyFifo>> mappedFileFifo;

    /// PDA DMABuffer object for the Ready FIFO
    boost::scoped_ptr<Pda::PdaDmaBuffer> bufferReadyFifo;

    /// Memory mapped data stored in the shared state file
    boost::scoped_ptr<FileSharedObject::FileSharedObject<CrorcSharedData>> crorcSharedData;

    /// Mapping from fifo page index to DMA buffer index
    std::vector<int> bufferPageIndexes;

    /// Array to keep track of read pages (false: wasn't read out, true: was read out).
    std::vector<bool> pageWasReadOut;

  private:

    void constructorCommon();
};

} // namespace Rorc
} // namespace AliceO2
