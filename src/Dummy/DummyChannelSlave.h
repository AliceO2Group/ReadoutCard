/// \file DummyChannelSlave.h
/// \brief Definition of the DummyChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ReadoutCard/ChannelSlaveInterface.h"
#include <array>

namespace AliceO2 {
namespace roc {

/// A dummy implementation of the ChannelSlaveInterface.
/// This exists so that the ReadoutCard module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// In the future, a dummy implementation could be a simulated card. Currently, methods of this implementation do
/// nothing besides print which method was called. Returned values are static and should not be used.
class DummyChannelSlave : public ChannelSlaveInterface
{
  public:

    DummyChannelSlave(const Parameters& parameters);
    virtual ~DummyChannelSlave();
    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;
    virtual CardType::type getCardType() override;
};

} // namespace roc
} // namespace AliceO2
