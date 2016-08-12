/// \file ChannelUtilityFactory.h
/// \brief Definition of the ChannelUtilityFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include "ChannelUtilityInterface.h"

namespace AliceO2 {
namespace Rorc {

/// Factory class for creating objects to access RORC channel's utility functions
class ChannelUtilityFactory
{
  public:
    static constexpr int DUMMY_SERIAL_NUMBER = -1;

    ChannelUtilityFactory();
    virtual ~ChannelUtilityFactory();

    /// Get a channel object with the given serial number and channel number.
    /// It is not yet implemented fully, currently it will just pick the
    /// first CRORC it comes across. If the PDA dependency is not available, a dummy implementation will be returned.
    /// \param serialNumber The serial number of the card. Passing 'DUMMY_SERIAL_NUMBER' returns a dummy implementation
    /// \param channelNumber The number of the channel to open.
    std::shared_ptr<ChannelUtilityInterface> getUtility(int serialNumber, int channelNumber);
};

} // namespace Rorc
} // namespace AliceO2

