///
/// \file DummyChannelMaster.h
/// \author Pascal Boeschoten
///

#pragma once

#include "RORC/ChannelMasterInterface.h"

namespace AliceO2 {
namespace Rorc {

/// A dummy implementation of the ChannelMasterInterface.
/// This exists so that the RORC module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// In the future, a dummy implementation could be a simulated card. Currently, methods of this implementation do
/// nothing besides print which method was called. Returned values are static and should not be used.
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
};

} // namespace Rorc
} // namespace AliceO2
