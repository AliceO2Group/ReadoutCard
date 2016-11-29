/// \file ResetLevel.cxx
/// \brief Implementation of the ResetLevel enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/GeneratorPattern.h"
#include <map>
#include "Util.h"

namespace AliceO2 {
namespace Rorc {
namespace {
static const std::map<GeneratorPattern::type, std::string> map = {
  { GeneratorPattern::Alternating,  "ALTERNATING" },
  { GeneratorPattern::Constant,     "CONSTANT" },
  { GeneratorPattern::Decremental,  "DECREMENTAL" },
  { GeneratorPattern::Flying0,      "FLYING_0" },
  { GeneratorPattern::Flying1,      "FLYING_1" },
  { GeneratorPattern::Incremental,  "INCREMENTAL" },
  { GeneratorPattern::Random,       "RANDOM" },
  { GeneratorPattern::Unknown,      "UNKNOWN" },
};

static const auto reverseMap = Util::reverseMap(map);
} // Anonymous namespace

std::string GeneratorPattern::toString(const GeneratorPattern::type& level)
{
  return Util::getValueFromMap(map, level);
}

GeneratorPattern::type GeneratorPattern::fromString(const std::string& string)
{
  return Util::getValueFromMap(reverseMap, string);
}

} // namespace Rorc
} // namespace AliceO2

