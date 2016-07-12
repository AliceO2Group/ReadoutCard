///
/// \file ChannelSlave.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "PdaDevice.h"
#include "PdaBar.h"
#include "RORC/ChannelSlaveInterface.h"
#include "RorcDeviceFinder.h"

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

    /// RORC device finder, gets the vendor & device ID of the card
    RorcDeviceFinder deviceFinder;

    /// PDA device objects
    PdaDevice pdaDevice;

    /// PDA BAR object
    PdaBar pdaBar;
};

} // namespace Rorc
} // namespace AliceO2
