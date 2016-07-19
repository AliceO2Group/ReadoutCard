///
/// \file ChannelSlave.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "RorcDevice.h"
#include "PdaBar.h"
#include "RORC/ChannelSlaveInterface.h"

namespace AliceO2 {
namespace Rorc {

/// Partially implements the ChannelSlaveInterface. It takes care of:
/// - Interprocess synchronization
/// - PDA-based functionality that is common to the CRORC and CRU.
class ChannelSlave: public ChannelSlaveInterface
{
  public:

    ChannelSlave(int serial, int channel);
    ~ChannelSlave();

    virtual uint32_t readRegister(int index);
    virtual void writeRegister(int index, uint32_t value);

  protected:

    /// Serial number of the device
    int serialNumber;

    /// DMA channel number
    int channelNumber;

    /// PDA device objects
    RorcDevice rorcDevice;

    /// PDA BAR object
    PdaBar pdaBar;
};

} // namespace Rorc
} // namespace AliceO2
