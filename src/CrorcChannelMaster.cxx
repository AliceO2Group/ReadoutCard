#include "CrorcChannelMaster.h"

namespace AliceO2 {
namespace Rorc {

CrorcChannelMaster::CrorcChannelMaster(int serial, int channel, const ChannelParameters& params)
: channelMaster(serial, channel, params)
{
}

} // namespace Rorc
} // namespace AliceO2
