/// \file CardType.cxx
/// \brief Implementation of the CardType enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/CardType.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace Rorc {
namespace {

static const auto converter = Utilities::makeEnumConverter<CardType::type>({
  { CardType::Unknown, "UNKNOWN" },
  { CardType::Crorc,   "CRORC" },
  { CardType::Cru,     "CRU" },
  { CardType::Dummy,   "DUMMY" },
});

} // Anonymous namespace

std::string CardType::toString(const CardType::type& type)
{
  return converter.toString(type);
}

CardType::type CardType::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace Rorc
} // namespace AliceO2
