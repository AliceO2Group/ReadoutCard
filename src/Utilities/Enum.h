/// \file Enum.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <stdexcept>
#include <utility>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/throw_exception.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Utilities {

template <typename Enum>
struct EnumConverter
{
    const std::vector<std::pair<Enum, std::string>> mapping;

    std::string toString(Enum e) const
    {
      for (const auto& pair : mapping) {
        if (e == pair.first) {
          return pair.second;
        }
      }
      BOOST_THROW_EXCEPTION(std::runtime_error("Invalid conversion"));
    }

    Enum fromString(const std::string& string) const
    {
      for (const auto& pair : mapping) {
        // Case-insensitive string comparison
        if (boost::iequals(string, pair.second)) {
          return pair.first;
        }
      }
      BOOST_THROW_EXCEPTION(std::runtime_error("Invalid conversion"));
    }
};

template <typename Enum>
EnumConverter<Enum> makeEnumConverter(std::vector<std::pair<Enum, std::string>> mapping)
{
  return EnumConverter<Enum>{mapping};
}


} // namespace Util
} // namespace Rorc
} // namespace AliceO2
