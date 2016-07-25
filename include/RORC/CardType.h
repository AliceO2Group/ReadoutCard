///
/// \file CardType.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once
#include <string>

namespace AliceO2 {
namespace Rorc {

/// Namespace for enum describing a RORC type, and supporting functions
struct CardType
{
    enum type
    {
      UNKNOWN, CRORC, CRU, DUMMY
    };

    /// Converts a CardType to a string
    static std::string toString(const CardType::type& type);

    /// Converts a string to a CardType
    static CardType::type fromString(const std::string& string);
};

} // namespace Rorc
} // namespace AliceO2
