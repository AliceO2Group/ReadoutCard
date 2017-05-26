/// \file CruDmaChannel.h
/// \brief Definition of the CruDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "DmaChannelPdaBase.h"
#include <memory>
#include <deque>
//#define BOOST_CB_ENABLE_DEBUG 1
#include <boost/circular_buffer.hpp>
#include "Cru/BarAccessor.h"
#include "Cru/FirmwareFeatures.h"
#include "ReadoutCard/Parameters.h"
#include "SuperpageQueue.h"

namespace AliceO2 {
namespace roc {

/// Extends DmaChannel object, and provides device-specific functionality
class CruDmaChannel final : public DmaChannelPdaBase
{
  public:

    CruDmaChannel(const Parameters& parameters);
    virtual ~CruDmaChannel() override;

    virtual CardType::type getCardType() override;

    virtual void pushSuperpage(Superpage) override;

    virtual int getTransferQueueAvailable() override;
    virtual int getReadyQueueSize() override;

    virtual Superpage getSuperpage() override;
    virtual Superpage popSuperpage() override;
    virtual void fillSuperpages() override;

    virtual bool injectError() override;
    virtual boost::optional<int32_t> getSerial() override;
    virtual boost::optional<float> getTemperature() override;
    virtual boost::optional<std::string> getFirmwareInfo() override;

    AllowedChannels allowedChannels();

  protected:

    virtual void deviceStartDma() override;
    virtual void deviceStopDma() override;
    virtual void deviceResetChannel(ResetLevel::type resetLevel) override;

  private:

    /// Max amount of superpages per link
    static constexpr size_t LINK_QUEUE_CAPACITY = Cru::MAX_SUPERPAGE_DESCRIPTORS;

    /// Max amount of superpages in the ready queue
    static constexpr size_t READY_QUEUE_CAPACITY = 32;

    /// Queue for one link
    using SuperpageQueue = boost::circular_buffer<Superpage>;

    /// Index into mLinks
    using LinkIndex = uint8_t;
    static_assert(std::numeric_limits<LinkIndex>::max() >= Cru::MAX_LINKS, "");

    /// ID for a link
    using LinkId = uint8_t;
    static_assert(std::numeric_limits<LinkId>::max() >= Cru::MAX_LINKS, "");

    struct Link
    {
        /// The link's FEE ID
        LinkId id = 0;

        /// The amount of superpages received from this link
        uint32_t superpageCounter = 0;

        /// The superpage queue
        SuperpageQueue queue {LINK_QUEUE_CAPACITY};
    };

    void initCru();
    void resetCru();
    void setBufferReady();
    void setBufferNonReady();

    Cru::BarAccessor getBar()
    {
      return Cru::BarAccessor(&mPdaBar);
    }

    Cru::BarAccessor getBar2()
    {
      return Cru::BarAccessor(&mPdaBar2);
    }

    /// BAR 0 is needed for DMA engine interaction and various other functions
    Pda::PdaBar mPdaBar;

    /// BAR 2 is needed to read serial number, temperature, etc.
    Pda::PdaBar mPdaBar2;

    /// Features of the firmware
    const FirmwareFeatures mFeatures;

    /// Vector of objects representing links
    std::vector<Link> mLinks;

    /// Queue to track in which order to push superpages to which link.
    /// At the start, it's filled round-robin with LinkIndexes.
    /// When a superpage is pushed, a LinkIndex is popped from the front of the queue and pushed to that link.
    /// When a superpage is popped, the LinkIndex of the link it was popped from is pushed to the back of the queue.
    boost::circular_buffer<LinkIndex> mLinkIndexQueue;

    SuperpageQueue mReadyQueue { READY_QUEUE_CAPACITY };

    // These variables are configuration parameters

    /// Reset level on initialization of channel
    ResetLevel::type mInitialResetLevel;

    /// Gives the type of loopback
    LoopbackMode::type mLoopbackMode;

    /// Enables the data generator
    bool mGeneratorEnabled;

    /// Data pattern for the data generator
    GeneratorPattern::type mGeneratorPattern;

    /// Random data size
    bool mGeneratorDataSizeRandomEnabled;

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

} // namespace roc
} // namespace AliceO2
