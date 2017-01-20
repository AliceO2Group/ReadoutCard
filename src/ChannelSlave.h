/// \file ChannelSlave.h
/// \brief Definition of the ChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "RorcDevice.h"
#include <boost/scoped_ptr.hpp>
#include "Pda/PdaBar.h"
#include "RORC/ChannelSlaveInterface.h"
#include "RORC/Parameters.h"

namespace AliceO2 {
namespace Rorc {

/// Partially implements the ChannelSlaveInterface. It takes care of:
/// - Interprocess synchronization
/// - PDA-based functionality that is common to the CRORC and CRU.
class ChannelSlave: public ChannelSlaveInterface
{
  public:

    ChannelSlave(const Parameters& parameters);
    virtual ~ChannelSlave();

    virtual uint32_t readRegister(int index);
    virtual void writeRegister(int index, uint32_t value);

  protected:

    /// Serial number of the device
    int mSerialNumber;

    /// DMA channel number
    int mChannelNumber;

    /// PDA device objects
    boost::scoped_ptr<RorcDevice> mRorcDevice;

    /// PDA BAR object
    boost::scoped_ptr<Pda::PdaBar> mPdaBar;
};

} // namespace Rorc
} // namespace AliceO2
