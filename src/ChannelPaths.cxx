/// \file ChannelPaths.cxx
/// \brief Implementation of the ChannelPaths class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelPaths.h"
#include <boost/format.hpp>

static const std::string DIR_SHAREDMEM("/dev/shm/alice_o2");
static const std::string FORMAT("%s/%s/channel_%i/%s");

namespace b = boost;

namespace AliceO2 {
namespace Rorc {

ChannelPaths::ChannelPaths(PciAddress pciAddress, int channel) : mPciAddress(pciAddress), mChannel(channel)
{
}

std::string ChannelPaths::makePath(std::string fileName) const
{
  return b::str(b::format(FORMAT) % DIR_SHAREDMEM % mPciAddress.toString() % mChannel % fileName);
}

std::string ChannelPaths::state() const
{
  return makePath("state");
}

std::string ChannelPaths::lock() const
{
  return makePath(".lock");
}

std::string ChannelPaths::fifo() const
{
  return makePath("fifo");
}

std::string ChannelPaths::namedMutex() const
{
  return b::str(b::format("alice_o2_rorc_%s_channel_%i.mutex") % mPciAddress.toString() % mChannel);
}

} // namespace Rorc
} // namespace AliceO2
