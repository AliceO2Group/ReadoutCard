/// \file Enum.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_UTILITIES_ENUM_H_
#define ALICEO2_SRC_READOUTCARD_UTILITIES_ENUM_H_

#include <stdexcept>
#include <utility>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/throw_exception.hpp>

namespace AliceO2 {
namespace roc {
namespace Utilities {

template <typename Enum>
struct EnumConverter
{
    const std::string typeName;
    const std::vector<std::pair<Enum, std::string>> mapping;

    std::string toString(Enum e) const
    {
      for (const auto& pair : mapping) {
        if (e == pair.first) {
          return pair.second;
        }
      }
      BOOST_THROW_EXCEPTION(std::runtime_error("Failed to convert " + typeName + " enum to string"));
    }

    Enum fromString(const std::string& string) const
    {
      for (const auto& pair : mapping) {
        // Case-insensitive string comparison
        if (boost::iequals(string, pair.second)) {
          return pair.first;
        }
      }
      BOOST_THROW_EXCEPTION(std::runtime_error("Failed to convert string to " + typeName + " enum"));
    }
};

template <typename Enum>
EnumConverter<Enum> makeEnumConverter(std::string typeName, std::vector<std::pair<Enum, std::string>> mapping)
{
  return EnumConverter<Enum>{typeName, mapping};
}


} // namespace Util
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_UTILITIES_ENUM_H_