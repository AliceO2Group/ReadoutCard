/// \file CrorcChannelMaster.h
/// \brief Definition of the CrorcChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <mutex>
#include <unordered_map>
#include <boost/circular_buffer_fwd.hpp>
#include <boost/scoped_ptr.hpp>
#include "ChannelMasterPdaBase.h"
#include "RORC/Parameters.h"
#include "PageManager.h"
#include "ReadyFifo.h"
#include "SuperpageQueue.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
class CrorcChannelMaster final : public ChannelMasterPdaBase
{
  public:

    CrorcChannelMaster(const Parameters& parameters);
    virtual ~CrorcChannelMaster() override;

    virtual CardType::type getCardType() override;

    virtual std::vector<uint32_t> utilityCopyFifo() override;
    virtual void utilityPrintFifo(std::ostream& os) override;
    virtual void utilitySetLedState(bool state) override;
    virtual void utilitySanityCheck(std::ostream& os) override;
    virtual void utilityCleanupState() override;
    virtual int utilityGetFirmwareVersion() override;
    virtual std::string utilityGetFirmwareVersionString() override;

    virtual void pushSuperpage(Superpage superpage) override;
    virtual int getSuperpageQueueCount() override;
    virtual int getSuperpageQueueAvailable() override;
    virtual int getSuperpageQueueCapacity() override;
    virtual Superpage getSuperpage() override;
    virtual Superpage popSuperpage() override;
    virtual void fillSuperpages() override;

    AllowedChannels allowedChannels();

  protected:

    virtual void deviceStartDma() override;
    virtual void deviceStopDma() override;
    virtual void deviceResetChannel(ResetLevel::type resetLevel) override;

    /// Name for the CRORC shared data object in the shared state file
    static std::string getCrorcSharedDataName()
    {
      return "CrorcChannelMasterSharedData";
    }

  private:

    /// This card's card type!
    static constexpr CardType::type CARD_TYPE = CardType::Crorc;

    /// Limits the number of superpages allowed in the queue
    static constexpr size_t MAX_SUPERPAGES = 32;

    /// Firmware FIFO Size
    static constexpr size_t FIFO_QUEUE_MAX = READYFIFO_ENTRIES;

    using SuperpageQueueType = SuperpageQueue<MAX_SUPERPAGES>;
    using SuperpageQueueEntry = SuperpageQueueType::SuperpageQueueEntry;

    /// Namespace for enum describing the status of a page's arrival
    struct DataArrivalStatus
    {
        enum type
        {
          NoneArrived,
          PartArrived,
          WholeArrived,
        };
    };

    uintptr_t getNextSuperpageBusAddress(const SuperpageQueueEntry& superpage);

    ReadyFifo* getFifoUser()
    {
      return reinterpret_cast<ReadyFifo*>(getFifoAddressUser());
    }

    ReadyFifo* getFifoBus()
    {
      return reinterpret_cast<ReadyFifo*>(getFifoAddressBus());
    }

    /// Enables data receiving in the RORC
    void startDataReceiving();

    /// Initializes and starts the data generator with the given parameters
    /// \param generatorParameters The parameters for the data generator.
    void startDataGenerator();

    /// Pushes a page to the CRORC's Free FIFO
    /// \param readyFifoIndex Index of the Ready FIFO to write the page's transfer status to
    /// \param pageBusAddress Address on the bus to push the page to
    void pushFreeFifoPage(int readyFifoIndex, uintptr_t pageBusAddress);

    /// Check if data has arrived
    DataArrivalStatus::type dataArrived(int index);

    /// Arms C-RORC data generator
    void crorcArmDataGenerator();

    /// Stops C-RORC data generator
    void crorcStopDataGenerator();

    /// Stops C-RORC data receiver
    void crorcStopDataReceiver();

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
    void crorcReset(int command);

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

    /// Starts pending DMA with given superpage for the initial pages
    void startPendingDma(SuperpageQueueEntry& superpage);

    /// Push a page into a superpage
    void pushIntoSuperpage(SuperpageQueueEntry& superpage);

    /// Get front index of FIFO
    int getFifoFront()
    {
      return (mFifoBack + mFifoSize) % READYFIFO_ENTRIES;
    };

    /// Back index of the firmware FIFO
    int mFifoBack = 0;

    /// Amount of elements in the firmware FIFO
    int mFifoSize = 0;

    /// Queue for superpages
    SuperpageQueueType mSuperpageQueue;

    /// Address of DMA buffer in userspace
    uintptr_t mDmaBufferUserspace = 0;

    /// Indicates deviceStartDma() was called, but DMA was not actually started yet. We do this because we need a
    /// superpage to actually start.
    bool mPendingDmaStart = false;

    // These variables are configuration parameters

    /// DMA page size
    const size_t mPageSize;

    /// Reset level on initialization of channel
    const ResetLevel::type mInitialResetLevel;

    /// Prevents sending the RDYRX and EOBTR commands. TODO This switch is implicitly set when data generator or the
    /// STBRD command is used (???)
    const bool mNoRDYRX;

    /// Enforces that the data reading is carried out with the Start Block Read (STBRD) command
    /// XXX Not sure if this should be a parameter...
    const bool mUseFeeAddress;

    /// Gives the type of loopback
    const LoopbackMode::type mLoopbackMode;

    /// Enables the data generator
    const bool mGeneratorEnabled;

    /// Data pattern for the data generator
    const GeneratorPattern::type mGeneratorPattern;

    /// Maximum number of events
    const int mGeneratorMaximumEvents;

    /// Initial value of the first data in a data block
    const uint32_t mGeneratorInitialValue;

    /// Sets the second word of each fragment when the data generator is used
    const uint32_t mGeneratorInitialWord;

    /// Random seed parameter in case the data generator is set to produce random data
    const int mGeneratorSeed;

    /// Length of data written to each page
    const size_t mGeneratorDataSize;

    /// Some timing parameter used during communications with the card
    long long int mLoopPerUsec = 0;

    /// Some timing parameters used during communications with the card
    double mPciLoopPerUsec = 0;

    /// Not sure
    int mRorcRevision = 0;

    /// Not sure
    int mSiuVersion = 0;

    /// Not sure
    int mDiuVersion = 0;
};

} // namespace Rorc
} // namespace AliceO2
