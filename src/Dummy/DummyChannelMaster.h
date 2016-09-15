///
/// \file DummyChannelMaster.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "RORC/Parameters.h"
#include "RORC/ChannelMasterInterface.h"
#include "ChannelUtilityInterface.h"
#include <array>

namespace AliceO2 {
namespace Rorc {

/// A dummy implementation of the ChannelMasterInterface.
/// This exists so that the RORC module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// In the future, a dummy implementation could be a simulated card.
/// Currently, most methods of this implementation do nothing besides print which method was called.
/// The getPage() function simulates incremental data generator output
class DummyChannelMaster : public ChannelMasterInterface, public ChannelUtilityInterface
{
  public:

    DummyChannelMaster(int serial, int channel, const Parameters::Map& params);
    virtual ~DummyChannelMaster();
    virtual void startDma() override;
    virtual void stopDma() override;
    virtual void resetCard(ResetLevel::type resetLevel) override;
    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;
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

  private:

    static constexpr size_t DUMMY_PAGE_SIZE = 4l * 1024l;

    int mPageCounter;
    std::array<int, DUMMY_PAGE_SIZE> mPageBuffer;
};

} // namespace Rorc
} // namespace AliceO2
