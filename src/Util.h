/// \file Util.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <map>
#include <set>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {
namespace Util {

/// Convenience function to reset a smart pointer
template <typename SmartPtr, typename ...Args>
void resetSmartPtr(SmartPtr& ptr, Args&&... args)
{
  ptr.reset(new typename SmartPtr::element_type(std::forward<Args>(args)...));
}

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
typename Map::mapped_type getValueFromMap(const Map& map, const typename Map::key_type& key)
{
  if (map.count(key) != 0) {
    return map.at(key);
  }
  BOOST_THROW_EXCEPTION(std::runtime_error("Invalid conversion"));
}

namespace _TieConvertImpl {
template <typename Container, int Index, typename ...Args>
struct Conv;

// Stop condition specialization: on a negative index
template <typename Container, typename ...Args>
struct Conv <Container, -1, Args...>
{
    void operator()(const Container&, Args&...)
    {
    }
};

// Recursive step specialization
template <typename Container, int Index, typename ...Args>
struct Conv
{
    void operator()(const Container& strings, Args&... args)
    {
      // Get the argument type
      using Arg = typename std::tuple_element<Index, std::tuple<Args...>>::type;
      // Use a tuple of the arguments to get static random access to the parameter pack
      auto& arg = std::get<Index>(std::tie(args...));
      // Convert the string to our newly found type and assign it to the argument
      arg = boost::lexical_cast<Arg>(strings[Index]);
      // Take a recursive step to the next argument
      Conv<Container, Index - 1, Args...>()(strings, args...);
    }
};
}

/// Takes each string in the container and assigns it to the argument in the corresponding position, converting it to
/// the appropriate type using boost::lexical_cast()
/// The vector must have a size at least as large as the amount of arguments to convert
/// Example:
/// std::vector<std::string> strings { "hello", "1.23", "42" };
/// std::string x;
/// double y;
/// int z;
/// convertAssign(strings, x, y, z);
///
/// \param strings A container of strings to convert. Must support random access. Size must be equal or greater than
///   amount of arguments.
/// \param args References to arguments that the conversions will be assigned to.
template <typename Container, typename ...Args>
void convertAssign(const Container& strings, Args&... args)
{
  if (strings.size() < sizeof...(args)) {
    BOOST_THROW_EXCEPTION(UtilException()
        << errinfo_rorc_error_message("Container size smaller than amount of arguments"));
  }
  _TieConvertImpl::Conv<Container, int(sizeof...(args)) - 1, Args...>()(strings, args...);
}

template <typename T1, typename T2>
void lexicalCast(const T1& from, T2& to)
{
  to = boost::lexical_cast<T2>(from);
}

inline uint32_t getLower32Bits(uint64_t x)
{
  return x & 0xFfffFfff;
}

inline uint32_t getUpper32Bits(uint64_t x)
{
  return (x >> 32) & 0xFfffFfff;
}

/// Sets the given function as the SIGINT handler
void setSigIntHandler(void(*function)(int));

/// Checks if there's a SIGINT handler installed (not sure if it actually works correctly)
bool isSigIntHandlerSet();

/// Like the "mkdir -p" command.
/// TODO Currently it actually calls that command.. not very portable, should refactor
void makeParentDirectories(const boost::filesystem::path& path);

/// Similar to the "touch" Linux command
void touchFile(const boost::filesystem::path& path);

/// Get the file system type of the given directory
std::string getFileSystemType(const boost::filesystem::path& path);

/// Check if the file system of the given directory is any of the types given in the set
/// \return pair with:
///   bool: true if it is any of types
///   std::string: the found type
std::pair<bool, std::string> isFileSystemTypeAnyOf(const boost::filesystem::path& path,
    const std::set<std::string>& types);

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
