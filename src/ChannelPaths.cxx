#include "ChannelPaths.h"
#include <boost/format.hpp>

static const std::string DIR_SHAREDMEM("/dev/shm/alice_o2");
static const std::string DIR_HUGEPAGES("/mnt/hugetlbfs/alice_o2");

namespace AliceO2 {
namespace Rorc {
namespace ChannelPaths {

namespace b = boost;
namespace bfs = boost::filesystem;

bfs::path pages(int serial, int channel)
{
  return b::str(b::format("%s/rorc_%i/channel_%i/pages") % DIR_HUGEPAGES % serial % channel);
}

bfs::path state(int serial, int channel)
{
  return b::str(b::format("%s/rorc_%i/channel_%i/state") % DIR_SHAREDMEM % serial % channel);
}

bfs::path lock(int serial, int channel)
{
  return b::str(b::format("%s/rorc_%i/channel_%i/.lock") % DIR_SHAREDMEM % serial % channel);
}

bfs::path fifo(int serial, int channel)
{
  return b::str(b::format("%s/rorc_%i/channel_%i/ready_fifo") % DIR_SHAREDMEM % serial % channel);
}

std::string namedMutex(int serial, int channel)
{
  return b::str(b::format("rorc_%i_channel_%i_mutex") % serial % channel);
}

} // namespace ChannelPaths
} // namespace Rorc
} // namespace AliceO2
