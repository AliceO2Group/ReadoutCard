///
/// \file DummyChannelMaster.h
/// \author Pascal Boeschoten
///

#pragma once

#include "RORC/ChannelMasterInterface.h"
#include <array>

namespace AliceO2 {
namespace Rorc {

/// A dummy implementation of the ChannelMasterInterface.
/// This exists so that the RORC module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// In the future, a dummy implementation could be a simulated card.
/// Currently, most methods of this implementation do nothing besides print which method was called.
/// The getPage() function simulates incremental data generator output
class DummyChannelMaster : public ChannelMasterInterface
{
  public:

    DummyChannelMaster(int serial, int channel, const ChannelParameters& params);
    ~DummyChannelMaster();
    virtual void startDma();
    virtual void stopDma();
    virtual void resetCard(ResetLevel::type resetLevel);
    virtual uint32_t readRegister(int index);
    virtual void writeRegister(int index, uint32_t value);
    virtual PageHandle pushNextPage();
    virtual bool isPageArrived(const PageHandle& handle);
    virtual Page getPage(const PageHandle& handle);
    virtual void markPageAsRead(const PageHandle& handle);
    virtual CardType::type getCardType();

  private:

    static constexpr size_t DUMMY_PAGE_SIZE = 4l * 1024l;

    int pageCounter;
    std::array<int, DUMMY_PAGE_SIZE> pageBuffer;
};

} // namespace Rorc
} // namespace AliceO2
