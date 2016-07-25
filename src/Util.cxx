///
/// \file Util.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "Util.h"
#include <signal.h>
#include <string.h>

namespace AliceO2 {
namespace Rorc {
namespace Util {

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

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
