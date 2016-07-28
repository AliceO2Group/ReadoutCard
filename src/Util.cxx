///
/// \file Util.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "Util.h"
#include <signal.h>
#include <cstring>
#include <fstream>
#include <boost/format.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Util {

namespace b = boost;
namespace bfs = boost::filesystem;

void setSigIntHandler(void(*function)(int))
{
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = function;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
}

bool isSigIntHandlerSet()
{
  struct sigaction sa;
  sigaction(SIGINT, NULL, &sa);
  return sa.sa_flags != 0;
}

void makeParentDirectories(const bfs::path& path)
{
  /// TODO Implement using boost::filesystem
  auto parent = path.parent_path();
  system(b::str(b::format("mkdir -p %s") % parent).c_str());
}

/// Similar to the "touch" Linux command
void touchFile(const bfs::path& path)
{
  std::ofstream ofs(path.c_str(), std::ios::app);
}

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
