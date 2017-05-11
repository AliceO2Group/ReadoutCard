/// \file LoopbackMode.cxx
/// \brief Implementation of the LoopbackMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ParameterTypes/LoopbackMode.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<LoopbackMode::type>({
  { LoopbackMode::None, "NONE" },
  { LoopbackMode::Internal, "INTERNAL" },
  { LoopbackMode::Diu, "DIU" },
  { LoopbackMode::Siu, "SIU" },
});

} // Anonymous namespace

bool LoopbackMode::isExternal(const LoopbackMode::type& mode)
{
  return mode == LoopbackMode::Siu || mode == LoopbackMode::Diu;
}

std::string LoopbackMode::toString(const LoopbackMode::type& mode)
{
  return converter.toString(mode);
}

LoopbackMode::type LoopbackMode::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2

