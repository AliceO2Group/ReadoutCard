/// \file ChannelFactory.h
/// \brief Definition of the ChannelFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CHANNELFACTORY_H_
#define ALICEO2_INCLUDE_READOUTCARD_CHANNELFACTORY_H_


#include "ReadoutCard/Parameters.h"
#include <memory>
#include <string>
#include "ReadoutCard/ChannelMasterInterface.h"
#include "ReadoutCard/ChannelSlaveInterface.h"

namespace AliceO2 {
namespace roc {

/// Factory class for creating objects to access and control card channels
class ChannelFactory
{
  public:
    static constexpr int DUMMY_SERIAL_NUMBER = -1;

    using MasterSharedPtr = ChannelMasterInterface::MasterSharedPtr;
    using SlaveSharedPtr = std::shared_ptr<ChannelSlaveInterface>;

    ChannelFactory();
    virtual ~ChannelFactory();

    /// Get a master channel object with the given serial number and channel number.
    /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
    /// \param parameters Parameters for the channel
    MasterSharedPtr getMaster(const Parameters& parameters);

    /// Get a slave channel object with the given serial number and channel number.
    /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
    /// \param parameters Parameters for the channel
    SlaveSharedPtr getSlave(const Parameters& parameters);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CHANNELFACTORY_H_
