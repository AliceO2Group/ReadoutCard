#include "RORC/CardType.h"
#include "Util.h"

namespace AliceO2 {
namespace Rorc {

static const std::map<CardType::type, std::string> typeMap = {
  { CardType::UNKNOWN, "UNKNOWN" },
  { CardType::CRORC, "CRORC" },
  { CardType::CRU, "CRU" },
};
static const auto typeMapReverse = Util::reverseMap(typeMap);

std::string CardType::toString(const CardType::type& level)
{
  return Util::getValue(typeMap, level);
}

CardType::type CardType::fromString(const std::string& string)
{
  return Util::getValue(typeMapReverse, string);
}

} // namespace Rorc
} // namespace AliceO2
