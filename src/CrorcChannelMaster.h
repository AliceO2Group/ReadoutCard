#pragma once

#include "ChannelMaster.h"

namespace AliceO2 {
namespace Rorc {

/// Wraps ChannelMaster object, and provides device-specific functionality
/// TODO Use inheritance instead?
/// TODO Implement...
class CrorcChannelMaster
{
  public:
    CrorcChannelMaster();
    ~CrorcChannelMaster();
  private:
    ChannelMaster channelMaster;
};

} // namespace Rorc
} // namespace AliceO2
