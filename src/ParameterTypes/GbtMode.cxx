/// \file GbtMode.cxx
/// \brief Implementation of the GbtMode enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/GbtMode.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<GbtMode::type>("GbtMode", {
  { GbtMode::Gbt, "Gbt" },
  { GbtMode::Wb,  "Wb" },
});

} // Anonymous namespace

std::string GbtMode::toString(const GbtMode::type& gbtMode)
{
  return converter.toString(gbtMode);
}

GbtMode::type GbtMode::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2
