/// \file ChannelSlave.h
/// \brief Definition of the ChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "RorcDevice.h"
#include "Pda/PdaBar.h"
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
    int mSerialNumber;

    /// DMA channel number
    int mChannelNumber;

    /// PDA device objects
    RorcDevice mRorcDevice;

    /// PDA BAR object
    Pda::PdaBar mPdaBar;
};

} // namespace Rorc
} // namespace AliceO2
