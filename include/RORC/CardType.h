#pragma once
#include <string>

namespace AliceO2 {
namespace Rorc {

/// Namespace for enum describing a RORC type, and supporting functions
namespace CardType
{
    enum type
    {
      UNKNOWN, CRORC, CRU, DUMMY
    };

    /// Converts a CardType to a string
    std::string toString(const CardType::type& level);

    /// Converts a string to a CardType
    CardType::type fromString(const std::string& string);
}

} // namespace Rorc
} // namespace AliceO2
