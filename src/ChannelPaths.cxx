/// \file ChannelPaths.cxx
/// \brief Implementation of the ChannelPaths class.
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
    : mCardType(cardType), mSerial(serial), mChannel(channel)
{
}

bfs::path ChannelPaths::state() const
{
  return b::str(b::format(FORMAT) % DIR_SHAREDMEM % CardType::toString(mCardType) % mSerial % mChannel % "state");
}

bfs::path ChannelPaths::lock() const
{
  return b::str(b::format(FORMAT) % DIR_SHAREDMEM % CardType::toString(mCardType) % mSerial % mChannel % ".lock");
}

bfs::path ChannelPaths::fifo() const
{
  return b::str(b::format(FORMAT) % DIR_SHAREDMEM % CardType::toString(mCardType) % mSerial % mChannel % "ready_fifo");
}

std::string ChannelPaths::namedMutex() const
{
  return b::str(b::format("card%s_serial_%i_channel_%i_mutex") % mCardType % mSerial % mChannel);
}

} // namespace Rorc
} // namespace AliceO2
