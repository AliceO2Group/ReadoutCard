/// \file ConvertAssign.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <boost/throw_exception.hpp>
#include <boost/lexical_cast.hpp>
#include "ExceptionInternal.h"


namespace AliceO2 {
namespace Rorc {
namespace Utilities {

namespace _convertAssignImpl {
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
template <typename Container = std::vector<std::string>, typename ...Args>
void convertAssign(const Container& strings, Args&... args)
{
  if (strings.size() < sizeof...(args)) {
    BOOST_THROW_EXCEPTION(UtilException()
        << ErrorInfo::Message("Container size smaller than amount of arguments"));
  }
  _convertAssignImpl::Conv<Container, int(sizeof...(args)) - 1, Args...>()(strings, args...);
}

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
