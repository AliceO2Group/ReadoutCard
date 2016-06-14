///
/// \file Exception.h
/// \author Pascal Boeschoten
///
/// Utilities for RORC exceptions
///

#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

namespace AliceO2 {
namespace Rorc {

class RorcException : public std::runtime_error
{
  public:
    inline RorcException(std::string file, int line, std::string message)
        : std::runtime_error(generateErrorString(file, line, message))
    {
    }

    inline static std::string generateErrorString(std::string file, int line, std::string message)
    {
      std::stringstream ss;
      ss << "Error @ " << file << ":" << line << " -> " << message;
      return ss.str();
    }
};

} // namespace Rorc
} // namespace AliceO2

#define ALICEO2_RORC_THROW_EXCEPTION(message) \
  throw AliceO2::Rorc::RorcException(__FILE__, __LINE__, message);
