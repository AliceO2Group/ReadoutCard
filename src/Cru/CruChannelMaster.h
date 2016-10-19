/// \file CruChannelMaster.h
/// \brief Definition of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include "BufferManager.h"
#include "ChannelMaster.h"
#include "CruFifoTable.h"
#include "RORC/Parameters.h"

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

    /// TEST
    virtual int _fillFifo(int maxFill = CRU_DESCRIPTOR_ENTRIES) override;
    /// TEST
    virtual boost::optional<_Page> _getPage() override;
    /// TEST
    virtual void _acknowledgePage() override;

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
    void resetCru();
    void setBufferReadyGuard();
    volatile uint32_t& bar(size_t index);
    _Page getPageFromIndex(int index);

    static constexpr CardType::type CARD_TYPE = CardType::Cru;

    std::unique_ptr<Util::GuardFunction> mBufferReadyGuard;

    /// Bus FIFO
    CruFifoTable* mFifoBus;

    /// Userspace FIFO
    CruFifoTable* mFifoUser;

    BufferManager<static_cast<int>(CRU_DESCRIPTOR_ENTRIES)> mBufferManager;
};

} // namespace Rorc
} // namespace AliceO2
