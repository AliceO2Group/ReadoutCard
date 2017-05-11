/// \file ResetLevel.cxx
/// \brief Implementation of the ResetLevel enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ParameterTypes/GeneratorPattern.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<GeneratorPattern::type>({
  { GeneratorPattern::Alternating,  "ALTERNATING" },
  { GeneratorPattern::Constant,     "CONSTANT" },
  { GeneratorPattern::Decremental,  "DECREMENTAL" },
  { GeneratorPattern::Flying0,      "FLYING_0" },
  { GeneratorPattern::Flying1,      "FLYING_1" },
  { GeneratorPattern::Incremental,  "INCREMENTAL" },
  { GeneratorPattern::Random,       "RANDOM" },
  { GeneratorPattern::Unknown,      "UNKNOWN" },
});

} // Anonymous namespace

std::string GeneratorPattern::toString(const GeneratorPattern::type& level)
{
  return converter.toString(level);
}

GeneratorPattern::type GeneratorPattern::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2

