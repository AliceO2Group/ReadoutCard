/// \file CruChannelMaster.h
/// \brief Definition of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ChannelMasterPdaBase.h"
#include <memory>
#include <queue>
#include <boost/circular_buffer_fwd.hpp>
#include "CruFifoTable.h"
#include "CruBarAccessor.h"
#include "PageManager.h"
#include "RORC/Parameters.h"
#include "SuperpageQueue.h"
#include "Utilities/GuardFunction.h"

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

    virtual void enqueueSuperpage(size_t offset, size_t size) override;
    virtual int getSuperpageQueueCount() override;
    virtual int getSuperpageQueueAvailable() override;
    virtual int getSuperpageQueueCapacity() override;
    virtual SuperpageStatus getSuperpageStatus() override;
    virtual SuperpageStatus popSuperpage() override;
    virtual void fillSuperpages() override;

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

    static constexpr size_t MAX_SUPERPAGES = 32;
    static constexpr size_t FIFO_QUEUE_MAX = CRU_DESCRIPTOR_ENTRIES; // Firmware FIFO Size

    int mFifoBack; ///< Back index of the firmware FIFO
    int mFifoSize; ///< Amount of elements in the firmware FIFO
    int getFifoFront()
    {
      return (mFifoBack + mFifoSize) % FIFO_QUEUE_MAX;
    };

    using SuperpageQueueType = SuperpageQueue<MAX_SUPERPAGES>;
    using SuperpageQueueEntry = SuperpageQueueType::SuperpageQueueEntry;
    SuperpageQueueType mSuperpageQueue;

    /// Acks that "should've" been issued before buffer was ready, but have to be postponed until after that
    int mPendingAcks = 0;

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

    /// Buffer readiness state. True means page descriptors have been filled, so the card can start transferring
    bool mBufferReady = false;

    int mInitialAcks = 0;

    void initFifo();
    void initCru();
    void resetCru();
    void setBufferReady();
    void setBufferNonReady();
//    int fillFifoNonLocking(int maxFill = CRU_DESCRIPTOR_ENTRIES);

    CruBarAccessor getBar()
    {
      return CruBarAccessor(getPdaBarPtr());
    }

    static constexpr CardType::type CARD_TYPE = CardType::Cru;

    CruFifoTable* getFifoUser()
    {
      return reinterpret_cast<CruFifoTable*>(getFifoAddressUser());
    }

    CruFifoTable* getFifoBus()
    {
      return reinterpret_cast<CruFifoTable*>(getFifoAddressBus());
    }

    void pushIntoSuperpage(SuperpageQueueEntry& superpage);
    volatile void* getNextSuperpageBusAddress(const SuperpageQueueEntry& superpage);
};

} // namespace Rorc
} // namespace AliceO2
