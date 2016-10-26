/// \file CruChannelMaster.h
/// \brief Definition of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include "ChannelMaster.h"
#include "CruFifoTable.h"
#include "PageManager.h"
#include "RORC/Parameters.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
class CruChannelMaster final : public ChannelMaster
{
  public:

    CruChannelMaster(int serial, int channel, const Parameters::Map& params);
    virtual ~CruChannelMaster() override;

    virtual void resetCard(ResetLevel::type resetLevel) override;
    virtual CardType::type getCardType() override;

    virtual std::vector<uint32_t> utilityCopyFifo() override;
    virtual void utilityPrintFifo(std::ostream& os) override;
    virtual void utilitySetLedState(bool state) override;
    virtual void utilitySanityCheck(std::ostream& os) override;
    virtual void utilityCleanupState() override;
    virtual int utilityGetFirmwareVersion() override;

    virtual int fillFifo(int maxFill = CRU_DESCRIPTOR_ENTRIES) override;
    virtual int getAvailableCount() override;
    virtual std::shared_ptr<Page> popPageInternal(const MasterSharedPtr& channel) override;
    virtual void freePage(const Page& page) override;


    AllowedChannels allowedChannels();

  protected:

    virtual void deviceStartDma() override;
    virtual void deviceStopDma() override;

    /// Name for the CRU shared data object in the shared state file
    static std::string getCruSharedDataName()
    {
      return "CruChannelMasterSharedData";
    }

  private:

    // TODO: refactor into ChannelMaster
    PageManager<CRU_DESCRIPTOR_ENTRIES> mPageManager;

    void initFifo();
    void initCru();
    void resetCru();
    void setBufferReadyGuard();
    volatile uint32_t& bar(size_t index);

    static constexpr CardType::type CARD_TYPE = CardType::Cru;

    std::unique_ptr<Util::GuardFunction> mBufferReadyGuard;

    /// Bus FIFO
    CruFifoTable* mFifoBus;

    /// Userspace FIFO
    CruFifoTable* mFifoUser;
};

} // namespace Rorc
} // namespace AliceO2
