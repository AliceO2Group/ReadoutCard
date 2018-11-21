/// \file DatapathMode.cxx
/// \brief Implementation of the DatapathMode enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/DatapathMode.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<DatapathMode::type>("DatapathMode", {
  { DatapathMode::Packet,       "Packet" },
  { DatapathMode::Continuous,   "Continuous" },
});

} // Anonymous namespace

std::string DatapathMode::toString(const DatapathMode::type& datapathMode)
{
  return converter.toString(datapathMode);
}

DatapathMode::type DatapathMode::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2
