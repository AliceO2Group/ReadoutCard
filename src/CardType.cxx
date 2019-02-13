/// \file CardType.cxx
/// \brief Implementation of the CardType enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// a test
#include "ReadoutCard/CardType.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<CardType::type>("CardType", {
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

} // namespace roc
} // namespace AliceO2
