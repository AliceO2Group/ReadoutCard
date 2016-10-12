/// \file CruChannelMaster.h
/// \brief Definition of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "RORC/Parameters.h"
#include "ChannelMaster.h"
#include <memory>
#include <array>
#include "CruFifoTable.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
/// XXX Note: this class is very under construction
class CruChannelMaster final : public ChannelMaster
{
  public:

    CruChannelMaster(int serial, int channel);
    CruChannelMaster(int serial, int channel, const Parameters::Map& params);
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

    void constructorCommon();
    void initFifo();
    void initCru();
    void setDescriptor(int pageIndex, int descriptorIndex);
    void resetBuffer();
    void resetCru();
    void resetPage(volatile uint32_t* page);
    void resetPage(volatile void* page);
    void setBufferReadyStatus(bool ready);
    void setBufferReadyGuard();
    volatile uint32_t& bar(size_t index);
    void acknowledgePage();

    static constexpr CardType::type CARD_TYPE = CardType::Cru;

    std::unique_ptr<Util::GuardFunction> mBufferReadyGuard;

    /// Userspace FIFO
    CruFifoTable* mFifoUser;

    /// Bus FIFO
    CruFifoTable* mFifoBus;

    /// Array to keep track of read pages (false: wasn't read out, true: was read out).
    std::vector<bool> mPageWasReadOut;

    /// Mapping from fifo page index to DMA buffer index
    std::vector<int> mBufferPageIndexes;

    int mFifoIndexWrite; ///< Index of next FIFO page available for writing
    int mFifoIndexRead; ///< Index of oldest non-free FIFO page
    int mBufferPageIndex; ///< Index of next DMA buffer page available for writing
};

} // namespace Rorc
} // namespace AliceO2
