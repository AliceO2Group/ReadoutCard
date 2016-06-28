#pragma once

#include "ChannelMaster.h"

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
/// TODO Factor CRORC-specific things out of ChannelMaster and into this
class CrorcChannelMaster
{
  public:
    CrorcChannelMaster(int serial, int channel, const ChannelParameters& params);
    ~CrorcChannelMaster();
  private:
    ChannelMaster channelMaster;
};

} // namespace Rorc
} // namespace AliceO2
