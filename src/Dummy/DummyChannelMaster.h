/// \file DummyChannelMaster.h
/// \brief Definition of the DummyChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <array>
#include <boost/scoped_ptr.hpp>
#include "ChannelMasterBase.h"

namespace AliceO2 {
namespace Rorc {

/// A dummy implementation of the ChannelMasterInterface.
/// This exists so that the RORC module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// It provides some basic simulation of page pushing and output.
class DummyChannelMaster final : public ChannelMasterBase
{
  public:

    DummyChannelMaster(const Parameters& parameters);
    virtual ~DummyChannelMaster();
    virtual void resetChannel(ResetLevel::type resetLevel) override;
    virtual void startDma() override;
    virtual void stopDma() override;
    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;
    virtual CardType::type getCardType() override;

    virtual boost::optional<std::string> getFirmwareInfo() override;
};

} // namespace Rorc
} // namespace AliceO2
