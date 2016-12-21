/// \file CruChannelMaster.h
/// \brief Definition of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ChannelMasterPdaBase.h"
#include <memory>
#include "CruFifoTable.h"
#include "CruBarAccessor.h"
#include "PageManager.h"
#include "RORC/Parameters.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
class CruChannelMaster final : public ChannelMasterPdaBase
{
  public:

    CruChannelMaster(const Parameters& parameters);
    virtual ~CruChannelMaster() override;

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
    virtual void deviceResetChannel(ResetLevel::type resetLevel) override;

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
    int fillFifoNonLocking(int maxFill = CRU_DESCRIPTOR_ENTRIES);

    CruBarAccessor getBar()
    {
      return CruBarAccessor(getPdaBarPtr());
    }

    static constexpr CardType::type CARD_TYPE = CardType::Cru;

    std::unique_ptr<Util::GuardFunction> mBufferReadyGuard;

    CruFifoTable* getFifoUser()
    {
      return reinterpret_cast<CruFifoTable*>(getFifoAddressUser());
    }

    CruFifoTable* getFifoBus()
    {
      return reinterpret_cast<CruFifoTable*>(getFifoAddressBus());
    }
};

} // namespace Rorc
} // namespace AliceO2
