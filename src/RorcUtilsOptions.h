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

/// Print a help message to standard output based on the given parameters
void printHelp(const UtilsDescription& utilDescription,
    const boost::program_options::options_description& optionsDescription);

/// Print an error and help message to standard output based on the given parameters
void printErrorAndHelp(const std::string& errorMessage, const UtilsDescription& utilsDescription,
    const boost::program_options::options_description& optionsDescription);

/// Create an options_description object, with the help switch already added
boost::program_options::options_description createOptionsDescription();

/// Get the variables_map object from the program arguments.
/// Will throw an exception if the program arguments given are invalid or the help switch is given.
boost::program_options::variables_map getVariablesMap(int argc, char** argv,
    const boost::program_options::options_description& od);

// Helper functions to add & get certain options in a standardized way
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
