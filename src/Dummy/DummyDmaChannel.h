/// \file DummyDmaChannel.h
/// \brief Definition of the DummyDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <array>
#include <boost/scoped_ptr.hpp>
#include "DmaChannelBase.h"

namespace AliceO2 {
namespace roc {

/// A dummy implementation of the DmaChannelInterface.
/// This exists so that the ReadoutCard module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// It provides some basic simulation of page pushing and output.
class DummyDmaChannel final : public DmaChannelBase
{
  public:

    DummyDmaChannel(const Parameters& parameters);
    virtual ~DummyDmaChannel();
    virtual void resetChannel(ResetLevel::type resetLevel) override;
    virtual void startDma() override;
    virtual void stopDma() override;
    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;
    virtual CardType::type getCardType() override;

    virtual int getTransferQueueAvailable() override;
    virtual int getReadyQueueSize() override;

    virtual boost::optional<std::string> getFirmwareInfo() override;
};

} // namespace roc
} // namespace AliceO2
