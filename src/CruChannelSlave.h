///
/// \file CruChannelSlave.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "ChannelSlave.h"

namespace AliceO2 {
namespace Rorc {

/// TODO Currently a placeholder
class CruChannelSlave: public ChannelSlave
{
  public:

    inline CruChannelSlave(int serial, int channel)
        : ChannelSlave(serial, channel)
    {
    }

    inline ~CruChannelSlave()
    {
    }
};

} // namespace Rorc
} // namespace AliceO2
