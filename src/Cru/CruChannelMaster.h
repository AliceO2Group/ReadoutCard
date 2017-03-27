/// \file CruChannelMaster.h
/// \brief Definition of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ChannelMasterPdaBase.h"
#include <memory>
#include <queue>
#include <boost/circular_buffer_fwd.hpp>
#include "CruBarAccessor.h"
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

    virtual void pushSuperpage(Superpage) override;
    virtual int getSuperpageQueueCount() override;
    virtual int getSuperpageQueueAvailable() override;
    virtual int getSuperpageQueueCapacity() override;
    virtual Superpage getSuperpage() override;
    virtual Superpage popSuperpage() override;
    virtual void fillSuperpages() override;

    virtual boost::optional<float> getTemperature() override;

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

    /// Limits the number of superpages allowed in the queue
    static constexpr size_t MAX_SUPERPAGES = 32;

    /// Firmware FIFO Size
    static constexpr size_t FIFO_QUEUE_MAX = 4;

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

    void initCru();
    void resetCru();
    void setBufferReady();
    void setBufferNonReady();
    void pushSuperpage(SuperpageQueueEntry& superpage);

    CruBarAccessor getBar()
    {
      return CruBarAccessor(getPdaBarPtr());
    }

    Pda::PdaBar* getPdaBar2Ptr();

    CruBarAccessor getBar2()
    {
      return CruBarAccessor(getPdaBar2Ptr());
    }

    static constexpr CardType::type CARD_TYPE = CardType::Cru;

    /// Get front index of FIFO
    int getFifoFront()
    {
      return (mFifoBack + mFifoSize) % FIFO_QUEUE_MAX;
    };

    int getFifoBack()
    {
      return mFifoBack;
    }

    void incrementFifoFront()
    {
      mFifoSize++;
    }

    void incrementFifoBack()
    {
      mFifoSize--;
      mFifoBack = (mFifoBack + 1) % FIFO_QUEUE_MAX;
    }

    /// Back index of the firmware FIFO
    int mFifoBack = 0;

    /// Amount of elements in the firmware FIFO
    int mFifoSize = 0;

    SuperpageQueueType mSuperpageQueue;

    /// BAR 2 is needed to read serial number, temperature, etc.
    /// We initialize it on demand
    std::unique_ptr<Pda::PdaBar> mPdaBar2;

    // These variables are configuration parameters

    /// Reset level on initialization of channel
    ResetLevel::type mInitialResetLevel;

    /// Gives the type of loopback
    LoopbackMode::type mLoopbackMode;

    /// Enables the data generator
    bool mGeneratorEnabled;

    /// Data pattern for the data generator
    GeneratorPattern::type mGeneratorPattern;

    /// Maximum number of events
    int mGeneratorMaximumEvents;

    /// Initial value of the first data in a data block
    uint32_t mGeneratorInitialValue;

    /// Sets the second word of each fragment when the data generator is used
    uint32_t mGeneratorInitialWord;

    /// Random seed parameter in case the data generator is set to produce random data
    int mGeneratorSeed;

    /// Length of data written to each page
    size_t mGeneratorDataSize;
};

} // namespace Rorc
} // namespace AliceO2
