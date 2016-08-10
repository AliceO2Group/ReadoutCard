///
/// \file CruChannelMaster.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "ChannelMaster.h"
#include <memory>
#include <array>
#include "CruFifoTable.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
/// XXX Note: this class is very under construction
class CruChannelMaster : public ChannelMaster
{
  public:

    CruChannelMaster(int serial, int channel);
    CruChannelMaster(int serial, int channel, const ChannelParameters& params);
    virtual ~CruChannelMaster() override;

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

    /// Name for the CRU shared data object in the shared state file
    static std::string getCruSharedDataName()
    {
      return "CruChannelMasterSharedData";
    }

  private:

    /// Persistent device state/data that resides in shared memory
    class SharedData
    {
      public:
        SharedData();

        /// Initialize the shared data fields
        void initialize();

        /// State of the initialization of the shared data
        InitializationState::type initializationState;

        /// Index of next page available for writing
        int fifoIndexWrite;

        /// Index of oldest non-free page
        int fifoIndexRead;

        /// Index to the next free page of the DMA buffer
        int pageIndex;
    };

    /// Memory mapped file containing the readyFifo
    boost::scoped_ptr<TypedMemoryMappedFile<CruFifoTable>> mappedFileFifo;

    /// PDA DMABuffer object for the readyFifo
    boost::scoped_ptr<Pda::PdaDmaBuffer> bufferFifo;

    /// Memory mapped data stored in the shared state file
    boost::scoped_ptr<FileSharedObject::FileSharedObject<SharedData>> cruSharedData;

    /// Counter for the amount of pages that have been requested.
    /// Since currently, the idea is to push 128 at a time, we wait until the client requests 128 pages...
    /// XXX This is of course a dirty hack and should be replaced when the CRU development matures
    int pendingPages;

    /// Array to keep track of read pages (false: wasn't read out, true: was read out).
    std::vector<bool> pageWasReadOut;

  private:

    void constructorCommon();

    static constexpr CardType::type CARD_TYPE = CardType::CRU;
};

} // namespace Rorc
} // namespace AliceO2
