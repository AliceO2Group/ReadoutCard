/// \file DownstreamData.cxx
/// \brief Implementation of the DownstreamData enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/DownstreamData.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<DownstreamData::type>("DownstreamData", {
  { DownstreamData::Ctp,      "Ctp" },
  { DownstreamData::Pattern,  "Pattern" },
  { DownstreamData::Midtrg,   "Midtrg" },
});

} // Anonymous namespace

std::string DownstreamData::toString(const DownstreamData::type& downstreamData)
{
  return converter.toString(downstreamData);
}

DownstreamData::type DownstreamData::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2
