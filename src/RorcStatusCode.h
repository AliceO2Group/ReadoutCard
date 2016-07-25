///
/// \file RorcStatusCode.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <string>

namespace AliceO2 {
namespace Rorc {

/// Get a string representing a RORC C API status code
std::string getRorcStatusString(int rorcStatusCode);

} // namespace Rorc
} // namespace AliceO2
