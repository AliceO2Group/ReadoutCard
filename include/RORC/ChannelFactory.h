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

    using MasterSharedPtr = ChannelMasterInterface::MasterSharedPtr;
    using SlaveSharedPtr = std::shared_ptr<ChannelSlaveInterface>;

    ChannelFactory();
    virtual ~ChannelFactory();

    /// Get a channel object with the given serial number and channel number.
    /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
    MasterSharedPtr getMaster(const Parameters& params);

    /// Get a channel object with the given serial number and channel number.
    /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
    SlaveSharedPtr getSlave(const Parameters& params);
};

} // namespace Rorc
} // namespace AliceO2
