///
/// \file Util.h
/// \author Pascal Boeschoten
///
/// It's the file with random useful utilities that don't really belong anywhere
///

#pragma once

#include <map>
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Util {

/// Flips a map around. Note that it will lead to data loss if multiple values of the original map are equal.
template <typename Map, typename ReverseMap = std::map<typename Map::mapped_type, typename Map::key_type>>
ReverseMap reverseMap(const Map& map)
{
  ReverseMap reverse;
  for (auto it = map.begin(); it != map.end(); ++it) {
    reverse.emplace(it->second, it->first);
  }
  return reverse;
}

/// Convenience function for implementing the enum to/from string functions
template <typename Map>
typename Map::mapped_type getValue(const Map& map, const typename Map::key_type& key)
{
  if (map.count(key) != 0) {
    return map.at(key);
  }
  BOOST_THROW_EXCEPTION(std::runtime_error("Invalid conversion"));
}

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
