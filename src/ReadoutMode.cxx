/// \file ReadoutMode.cxx
/// \brief Implementation of the ReadoutMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/ReadoutMode.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace Rorc {
namespace {

static const auto converter = Utilities::makeEnumConverter<ReadoutMode::type>({
  { ReadoutMode::Continuous, "CONTINUOUS" },
});

} // Anonymous namespace

std::string ReadoutMode::toString(const ReadoutMode::type& mode)
{
  return converter.toString(mode);
}

ReadoutMode::type ReadoutMode::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace Rorc
} // namespace AliceO2

