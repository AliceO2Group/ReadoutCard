/// \file RorcStatusCode.h
/// \brief Definition of the Rorc::StatusCode functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <string>

namespace AliceO2 {
namespace Rorc {
namespace StatusCode {

/// Get a string representing a RORC C API status code
std::string getString(int statusCode);

} // namespace StatusCode
} // namespace Rorc
} // namespace AliceO2
