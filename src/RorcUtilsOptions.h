///
/// \file RorcUtilsOptions.h
/// \author Pascal Boeschoten
///
/// Functions functions for the RORC utilities to handle program options
///

#pragma once

#include <boost/program_options.hpp>
#include "RorcUtilsDescription.h"

namespace AliceO2 {
namespace Rorc {
namespace Util {
namespace Options {

struct UtilDescription
{
    std::string name; ///< Name of the utility
    std::string description; ///< Short description
    std::string usage; ///< Usage example
};

void printHelp(const UtilDescription& utilDescription,
    const boost::program_options::options_description& optionsDescription);

void printErrorAndHelp(const std::string& errorMessage, const UtilsDescription& utilsDescription,
    const boost::program_options::options_description& optionsDescription);

boost::program_options::options_description createOptionsDescription();
boost::program_options::variables_map getVariablesMap(int argc, char** argv,
    const boost::program_options::options_description& od);

void addOptionHelp(boost::program_options::options_description& od);
void addOptionRegisterAddress(boost::program_options::options_description& od);
void addOptionRegisterRange(boost::program_options::options_description& od);
void addOptionChannel(boost::program_options::options_description& od);
void addOptionSerialNumber(boost::program_options::options_description& od);

int getOptionRegisterAddress(const boost::program_options::variables_map& variablesMap);
int getOptionChannel(const boost::program_options::variables_map& variablesMap);
int getOptionSerialNumber(const boost::program_options::variables_map& variablesMap);
int getOptionRegisterRange(const boost::program_options::variables_map& variablesMap);

} // namespace Options
} // namespace Util
} // namespace Rorc
} // namespace AliceO2
