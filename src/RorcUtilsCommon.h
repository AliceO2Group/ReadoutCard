///
/// \file RorcUtilsCommon.h
/// \author Pascal Boeschoten
///
/// Some common functions for the RORC utilities.
///

#pragma once

#include <boost/program_options.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Util {
namespace Common {

std::string make32bitString(uint32_t bits);
std::string make32hexString(uint32_t bits);
std::string makeRegisterAddressString(int address);
std::string makeRegisterString(int address, uint32_t value);

void printHelp (std::string utilName, std::string utilDescription, std::string usageExample,
    const boost::program_options::options_description& optionsDescription);

void printErrorAndHelp(std::string errorMessage, std::string utilName, std::string utilDescription,
    std::string usageExample, const boost::program_options::options_description& optionsDescription);

} // namespace Common
} // namespace Util
} // namespace Rorc
} // namespace AliceO2
