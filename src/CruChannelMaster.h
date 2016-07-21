///
/// \file CruChannelMaster.h
/// \author Pascal Boeschoten
///

#pragma once

#include "ChannelMaster.h"
#include <array>

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
class CruChannelMaster : public ChannelMaster
{
  public:

    CruChannelMaster(int serial, int channel, const ChannelParameters& params);
    ~CruChannelMaster();
    virtual void resetCard(ResetLevel::type resetLevel);
    virtual PageHandle pushNextPage();
    virtual bool isPageArrived(const PageHandle& handle);
    virtual Page getPage(const PageHandle& handle);
    virtual void markPageAsRead(const PageHandle& handle);
    virtual CardType::type getCardType();

  protected:

    virtual void deviceStartDma();
    virtual void deviceStopDma();

    /// Name for the CRU shared data object in the shared state file
    inline static const char* crorcSharedDataName()
    {
      return "CruChannelMasterSharedData";
    }

  private:

    static constexpr size_t CRU_DESCRIPTOR_ENTRIES = 128l;

    /// A class representing the CRU status and descriptor tables
    struct CruFifoTable
    {
        /// A class representing a CRU status table entry
        struct StatusEntry
        {
            volatile uint32_t status;
        };

        /// A class representing a CRU descriptor table entry
        struct DescriptorEntry
        {

            /// Low 32 bits of the DMA source address on the card
            uint32_t srcLow;

            /// High 32 bits of the DMA source address on the card
            uint32_t srcHigh;

            /// Low 32 bits of the DMA destination address on the bus
            uint32_t dstLow;

            /// High 32 bits of the DMA destination address on the bus
            uint32_t dstHigh;

            /// Control register
            uint32_t ctrl;

            /// Reserved field 1
            uint32_t reserved1;

            /// Reserved field 2
            uint32_t reserved2;

            /// Reserved field 3
            uint32_t reserved3;
        };

        /// Array of status entries
        std::array<StatusEntry, CRU_DESCRIPTOR_ENTRIES> statusEntries;

        /// Array of descriptor entries
        std::array<DescriptorEntry, CRU_DESCRIPTOR_ENTRIES> descriptorEntries;
    };

    /// Persistent device state/data that resides in shared memory
    class CrorcSharedData
    {
      public:
        CrorcSharedData();

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
    TypedMemoryMappedFile<CruFifoTable> mappedFileFifo;

    /// PDA DMABuffer object for the readyFifo
    PdaDmaBuffer bufferFifo;

    /// Memory mapped data stored in the shared state file
    FileSharedObject::FileSharedObject<CrorcSharedData> crorcSharedData;

    /// Counter for the amount of pages that have been requested.
    /// Since currently, the idea is to push 128 at a time, we wait until the client requests 128 pages...
    /// XXX This is of course a dirty hack and should be replaced when the CRU development matures
    int pendingPages;

    /// Array to keep track of read pages (false: wasn't read out, true: was read out).
    std::vector<bool> pageWasReadOut;
};

} // namespace Rorc
} // namespace AliceO2
