/// \file CardType.cxx
/// \brief Implementation of the CardType enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/CardType.h"
#include "Util.h"

namespace AliceO2 {
namespace Rorc {

namespace {

static const std::map<CardType::type, std::string> typeMap = {
  { CardType::Unknown, "UNKNOWN" },
  { CardType::Crorc, "CRORC" },
  { CardType::Cru, "CRU" },
  { CardType::Dummy, "DUMMY" },
};

static const std::map<std::string, CardType::type> typeMapReverse = Util::reverseMap(typeMap);

} // Anonymous namespace

std::string CardType::toString(const CardType::type& type)
{
  return Util::getValueFromMap(typeMap, type);
}

CardType::type CardType::fromString(const std::string& string)
{
  return Util::getValueFromMap(typeMapReverse, string);
}

} // namespace Rorc
} // namespace AliceO2
