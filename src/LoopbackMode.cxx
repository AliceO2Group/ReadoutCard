/// \file LoopbackMode.cxx
/// \brief Implementation of the LoopbackMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/LoopbackMode.h"
#include <map>
#include "Util.h"

namespace AliceO2 {
namespace Rorc {
namespace {
static const std::map<LoopbackMode::type, std::string> loopbackModeMap {
  { LoopbackMode::None, "NONE" },
  { LoopbackMode::Rorc, "RORC" },
  { LoopbackMode::Diu, "DIU" },
  { LoopbackMode::Siu, "SIU" },
};

static const auto loopbackModeMapReverse = Util::reverseMap(loopbackModeMap);
} // Anonymous namespace

bool LoopbackMode::isExternal(const LoopbackMode::type& mode)
{
  return mode == LoopbackMode::Siu || mode == LoopbackMode::Diu;
}

std::string LoopbackMode::toString(const LoopbackMode::type& mode)
{
  return Util::getValueFromMap(loopbackModeMap, mode);
}

LoopbackMode::type LoopbackMode::fromString(const std::string& string)
{
  return Util::getValueFromMap(loopbackModeMapReverse, string);
}

} // namespace Rorc
} // namespace AliceO2

