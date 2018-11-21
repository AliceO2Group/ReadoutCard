/// \file GbtMux.cxx
/// \brief Implementation of the GbtMux enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/GbtMux.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<GbtMux::type>("GbtMux", {
  { GbtMux::Ttc, "Ttc" },
  { GbtMux::Ddg, "Ddg" },
  { GbtMux::Sc,  "Sc" },
});

} // Anonymous namespace

std::string GbtMux::toString(const GbtMux::type& gbtMux)
{
  return converter.toString(gbtMux);
}

GbtMux::type GbtMux::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2
