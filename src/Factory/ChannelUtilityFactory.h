/// \file ChannelUtilityFactory.h
/// \brief Definition of the ChannelUtilityFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include "ChannelUtilityInterface.h"
#include "RORC/Parameters.h"

namespace AliceO2 {
namespace Rorc {

/// Factory class for creating objects to access RORC channel's utility functions
class ChannelUtilityFactory
{
  public:
    static constexpr int DUMMY_SERIAL_NUMBER = -1;

    using UtilitySharedPtr = std::shared_ptr<ChannelUtilityInterface>;

    ChannelUtilityFactory();
    virtual ~ChannelUtilityFactory();

    /// Get a channel object with the given serial number and channel number that gives access to utility functions
    /// Passing 'DUMMY_SERIAL_NUMBER' returns a dummy implementation
    UtilitySharedPtr getUtility(const Parameters& params);
};

} // namespace Rorc
} // namespace AliceO2

