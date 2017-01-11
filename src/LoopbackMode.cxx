/// \file LoopbackMode.cxx
/// \brief Implementation of the LoopbackMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/LoopbackMode.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace Rorc {
namespace {

static const auto converter = Utilities::makeEnumConverter<LoopbackMode::type>({
  { LoopbackMode::None, "NONE" },
  { LoopbackMode::Rorc, "RORC" },
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

} // namespace Rorc
} // namespace AliceO2

