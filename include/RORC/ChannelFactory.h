///
/// \file ChannelFactory.h
/// \author Pascal Boeschoten
///

#pragma once

#include <memory>
#include "RORC/ChannelMasterInterface.h"
#include "RORC/ChannelSlaveInterface.h"
#include "RORC/ChannelParameters.h"

namespace AliceO2 {
namespace Rorc {

/// Factory class for creating objects to access and control RORC channels
class ChannelFactory
{
  public:
    static constexpr int DUMMY_SERIAL_NUMBER = -1;

    ChannelFactory();
    virtual ~ChannelFactory();

    /// Get a channel object with the given serial number and channel number.
    /// It is not yet implemented fully, currently it will just pick the
    /// first CRORC it comes across. If the PDA dependency is not available, a dummy implementation will be returned.
    /// \param serialNumber The serial number of the card. Passing 'DUMMY_SERIAL_NUMBER' returns a dummy implementation
    /// \param channelNumber The number of the channel to open.
    /// \param params Parameters to pass onto the ChannelMasterInterface implementation
    std::shared_ptr<ChannelMasterInterface> getMaster(int serialNumber, int channelNumber, const ChannelParameters& params);

    /// Get a channel object with the given serial number and channel number.
    /// It is not yet implemented fully, currently it will just pick the
    /// first CRORC it comes across. If the PDA dependency is not available, a dummy implementation will be returned.
    /// \param serialNumber The serial number of the card. Passing 'DUMMY_SERIAL_NUMBER' returns a dummy implementation
    /// \param channelNumber The number of the channel to open.
    std::shared_ptr<ChannelSlaveInterface> getSlave(int serialNumber, int channelNumber);
};

} // namespace Rorc
} // namespace AliceO2

