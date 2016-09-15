/// \file ResetLevel.cxx
/// \brief Implementation of the ResetLevel enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/ResetLevel.h"
#include <map>
#include "Util.h"

namespace AliceO2 {
namespace Rorc {

static const std::map<ResetLevel::type, std::string> resetLevelMap = {
  { ResetLevel::Nothing, "NOTHING" },
  { ResetLevel::Rorc, "RORC" },
  { ResetLevel::RorcDiu, "RORC_DIU" },
  { ResetLevel::RorcDiuSiu, "RORC_DIU_SIU" },
};

static const auto resetLevelMapReverse = Util::reverseMap(resetLevelMap);

bool ResetLevel::includesExternal(const ResetLevel::type& mode)
{
  return mode == ResetLevel::RorcDiu || mode == ResetLevel::RorcDiuSiu;
}

std::string ResetLevel::toString(const ResetLevel::type& level)
{
  return Util::getValueFromMap(resetLevelMap, level);
}

ResetLevel::type ResetLevel::fromString(const std::string& string)
{
  return Util::getValueFromMap(resetLevelMapReverse, string);
}

} // namespace Rorc
} // namespace AliceO2

