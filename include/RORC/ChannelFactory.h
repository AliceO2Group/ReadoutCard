/// \file ChannelFactory.h
/// \brief Definition of the ChannelFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "RORC/Parameters.h"
#include <memory>
#include <string>
#include "RORC/ChannelMasterInterface.h"
#include "RORC/ChannelSlaveInterface.h"

namespace AliceO2 {
namespace Rorc {

/// Factory class for creating objects to access and control card channels
class ChannelFactory
{
  public:
    static constexpr int DUMMY_SERIAL_NUMBER = -1;

    using MasterSharedPtr = std::shared_ptr<ChannelMasterInterface>;
    using SlaveSharedPtr = std::shared_ptr<ChannelSlaveInterface>;

    ChannelFactory();
    virtual ~ChannelFactory();

    /// Get a channel object with the given serial number and channel number.
    /// It is not yet implemented fully, currently it will just pick the
    /// first CRORC it comes across. If the PDA dependency is not available, a dummy implementation will be returned.
    ///
    /// \param serialNumber The serial number of the card. Passing 'DUMMY_SERIAL_NUMBER' returns a dummy implementation
    /// \param channelNumber The number of the channel to open.
    /// \param params Parameters to pass onto the ChannelMasterInterface implementation
    MasterSharedPtr getMaster(int serialNumber, int channelNumber, const Parameters::Map& params);

    /// Get a channel object with the given serial number and channel number.
    /// It is not yet implemented fully, currently it will just pick the
    /// first CRORC it comes across. If the PDA dependency is not available, a dummy implementation will be returned.
    /// Since this getMaster() version does not pass any parameters along, the ChannelMaster object will require
    /// previous state to be available: a previous successful construction and destruction.
    ///
    /// \param serialNumber The serial number of the card. Passing 'DUMMY_SERIAL_NUMBER' returns a dummy implementation
    /// \param channelNumber The number of the channel to open.
    MasterSharedPtr getMaster(int serialNumber, int channelNumber);

    /// Get a channel object with the given serial number and channel number.
    /// It is not yet implemented fully, currently it will just pick the
    /// first CRORC it comes across. If the PDA dependency is not available, a dummy implementation will be returned.
    ///
    /// \param serialNumber The serial number of the card. Passing 'DUMMY_SERIAL_NUMBER' returns a dummy implementation
    /// \param channelNumber The number of the channel to open.
    SlaveSharedPtr getSlave(int serialNumber, int channelNumber);
};

} // namespace Rorc
} // namespace AliceO2
