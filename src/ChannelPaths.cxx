/// \file ChannelPaths.cxx
/// \brief Implementation of the ChannelPaths class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelPaths.h"
#include <boost/format.hpp>

namespace b = boost;

namespace AliceO2 {
namespace Rorc {
namespace {
static const char* DIR_SHAREDMEM = "/dev/shm/alice_o2/rorc";
static const char* DIR_HUGEPAGE = "/dev/hugepages/alice_o2/rorc";
static const char* FORMAT = "%s/%s/channel_%i/%s";
}

ChannelPaths::ChannelPaths(PciAddress pciAddress, int channel) : mPciAddress(pciAddress), mChannel(channel)
{
}

std::string ChannelPaths::makePath(std::string fileName, const char* directory) const
{
  return b::str(b::format(FORMAT) % directory % mPciAddress.toString() % mChannel % fileName);
}

std::string ChannelPaths::lock() const
{
  return makePath(".lock", DIR_SHAREDMEM);
}

std::string ChannelPaths::fifo() const
{
  return makePath("fifo", DIR_HUGEPAGE);
}

std::string ChannelPaths::namedMutex() const
{
  return b::str(b::format("alice_o2_rorc_%s_channel_%i.mutex") % mPciAddress.toString() % mChannel);
}

} // namespace Rorc
} // namespace AliceO2
