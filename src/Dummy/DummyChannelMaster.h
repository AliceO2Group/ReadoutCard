/// \file DummyChannelMaster.h
/// \brief Definition of the DummyChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <array>
#include "ChannelUtilityInterface.h"
#include "PageManager.h"
#include "RORC/Parameters.h"
#include "RORC/ChannelMasterInterface.h"

namespace AliceO2 {
namespace Rorc {

/// A dummy implementation of the ChannelMasterInterface.
/// This exists so that the RORC module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// In the future, a dummy implementation could be a simulated card.
/// Currently, most methods of this implementation do nothing besides print which method was called.
/// The getPage() function simulates incremental data generator output
class DummyChannelMaster final : public ChannelMasterInterface, public ChannelUtilityInterface
{
  public:

    DummyChannelMaster(int serial, int channel, const Parameters::Map& params);
    virtual ~DummyChannelMaster();
    virtual void resetChannel(ResetLevel::type resetLevel) override;
    virtual void startDma() override;
    virtual void stopDma() override;
    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;
    virtual CardType::type getCardType() override;

    virtual int fillFifo(int maxFill) override;
    virtual int getAvailableCount() override;
    virtual std::shared_ptr<Page> popPageInternal(const MasterSharedPtr& channel) override;
    virtual void freePage(const Page& page) override;

    virtual std::vector<uint32_t> utilityCopyFifo() override;
    virtual void utilityPrintFifo(std::ostream& os) override;
    virtual void utilitySetLedState(bool state) override;
    virtual void utilitySanityCheck(std::ostream& os) override;
    virtual void utilityCleanupState() override;
    virtual int utilityGetFirmwareVersion() override;

  private:
    static constexpr size_t FIFO_CAPACITY = 128;
    PageManager<FIFO_CAPACITY> mPageManager;

    size_t mBufferSize;
    size_t mPageSize;
    size_t mMaxPages;
    int mPageCounter;
    std::vector<char> mPageBuffer;
};

} // namespace Rorc
} // namespace AliceO2
