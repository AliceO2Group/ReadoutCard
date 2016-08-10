/// \file ChannelPaths.h
/// \brief Implementation of ChannelPaths
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelPaths.h"
#include <boost/format.hpp>

static const std::string DIR_SHAREDMEM("/dev/shm/alice_o2");
static const std::string DIR_HUGEPAGES("/mnt/hugetlbfs/alice_o2");
static const std::string FORMAT("%s/card_%s/serial_%i/channel_%i/%s");

namespace AliceO2 {
namespace Rorc {

namespace b = boost;
namespace bfs = boost::filesystem;

ChannelPaths::ChannelPaths(CardType::type cardType, int serial, int channel)
    : cardType(cardType), serial(serial), channel(channel)
{
}

bfs::path ChannelPaths::pages() const
{
  return b::str(b::format(FORMAT) % DIR_HUGEPAGES % CardType::toString(cardType) % serial % channel % "pages");
}

bfs::path ChannelPaths::state() const
{
  return b::str(b::format(FORMAT) % DIR_SHAREDMEM % CardType::toString(cardType) % serial % channel % "state");
}

bfs::path ChannelPaths::lock() const
{
  return b::str(b::format(FORMAT) % DIR_SHAREDMEM % CardType::toString(cardType) % serial % channel % ".lock");
}

bfs::path ChannelPaths::fifo() const
{
  return b::str(b::format(FORMAT) % DIR_SHAREDMEM % CardType::toString(cardType) % serial % channel % "ready_fifo");
}

std::string ChannelPaths::namedMutex() const
{
  return b::str(b::format("card%s_serial_%i_channel_%i_mutex") % cardType % serial % channel);
}

} // namespace Rorc
} // namespace AliceO2
