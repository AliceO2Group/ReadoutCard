#pragma once
#include <string>

namespace AliceO2 {
namespace Rorc {

struct CardType
{
    enum type
    {
      UNKNOWN, CRORC, CRU
    };

    /// Converts a CardType to a string
    static std::string toString(const CardType::type& level);

    /// Converts a string to a CardType
    static CardType::type fromString(const std::string& string);
};

} // namespace Rorc
} // namespace AliceO2
