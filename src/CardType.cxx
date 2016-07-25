///
/// \file CardType.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "RORC/CardType.h"
#include "Util.h"

namespace AliceO2 {
namespace Rorc {

static const std::map<CardType::type, std::string> typeMap = {
  { CardType::UNKNOWN, "UNKNOWN" },
  { CardType::CRORC, "CRORC" },
  { CardType::CRU, "CRU" },
  { CardType::DUMMY, "DUMMY" },
};
static const auto typeMapReverse = Util::reverseMap(typeMap);

std::string CardType::toString(const CardType::type& type)
{
  return Util::getValue(typeMap, type);
}

CardType::type CardType::fromString(const std::string& string)
{
  return Util::getValue(typeMapReverse, string);
}

} // namespace Rorc
} // namespace AliceO2
