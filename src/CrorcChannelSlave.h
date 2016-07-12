///
/// \file CrorcChannelSlave.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "ChannelSlave.h"

namespace AliceO2 {
namespace Rorc {

/// TODO Currently a placeholder
class CrorcChannelSlave: public ChannelSlave
{
  public:

    inline CrorcChannelSlave(int serial, int channel)
        : ChannelSlave(serial, channel)
    {
    }

    inline ~CrorcChannelSlave()
    {
    }
};

} // namespace Rorc
} // namespace AliceO2
