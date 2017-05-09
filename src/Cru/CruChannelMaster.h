/// \file CruChannelMaster.h
/// \brief Definition of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ChannelMasterPdaBase.h"
#include <memory>
#include <queue>
#include <boost/circular_buffer_fwd.hpp>
#include "Cru/BarAccessor.h"
#include "RORC/Parameters.h"
#include "SuperpageQueue.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
class CruChannelMaster final : public ChannelMasterPdaBase
{
  public:

    CruChannelMaster(const Parameters& parameters);
    virtual ~CruChannelMaster() override;

    virtual CardType::type getCardType() override;

    virtual void pushSuperpage(Superpage) override;
    virtual int getSuperpageQueueCount() override;
    virtual int getSuperpageQueueAvailable() override;
    virtual int getSuperpageQueueCapacity() override;
    virtual Superpage getSuperpage() override;
    virtual Superpage popSuperpage() override;
    virtual void fillSuperpages() override;

    virtual boost::optional<float> getTemperature() override;
    virtual boost::optional<std::string> getFirmwareInfo() override;

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

    // Max amount of superpages per link
    static constexpr size_t LINK_MAX_SUPERPAGES = 32;

    // Queue for one link
    using LinkQueue = boost::circular_buffer<Superpage>;

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

    Cru::BarAccessor getBar()
    {
      return Cru::BarAccessor(getPdaBarPtr());
    }

    Pda::PdaBar* getPdaBar2Ptr();

    Cru::BarAccessor getBar2()
    {
      return Cru::BarAccessor(getPdaBar2Ptr());
    }

    static constexpr CardType::type CARD_TYPE = CardType::Cru;

    LinkQueue mLinkQueue { LINK_MAX_SUPERPAGES };
    uint32_t mLinkSuperpageCounter { 0 };

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
