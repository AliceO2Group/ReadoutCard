/// \file Clock.cxx
/// \brief Implementation of the Clock enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/Clock.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<Clock::type>("Clock", {
  { Clock::Local, "Local" },
  { Clock::Ttc,   "Ttc" },
});

} // Anonymous namespace

std::string Clock::toString(const Clock::type& clock)
{
  return converter.toString(clock);
}

Clock::type Clock::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2

