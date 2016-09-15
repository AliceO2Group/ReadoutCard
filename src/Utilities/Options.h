///
/// \file Options.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Functions for the RORC utilities to handle program options
///

#pragma once

#include <boost/program_options.hpp>
#include <boost/exception/all.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/Parameters.h"
#include <Utilities/UtilsDescription.h>
#include <iostream>
#include "RORC/Exception.h"
#include "ChannelParameters.h"

namespace AliceO2 {
namespace Rorc {
namespace Utilities {
namespace Options {

/// Create an options_description object, with the help switch already added
boost::program_options::options_description createOptionsDescription();

/// Get the variables_map object from the program arguments.
/// Will throw an exception if the program arguments given are invalid or the help switch is given.
boost::program_options::variables_map getVariablesMap(int argc, char** argv,
    const boost::program_options::options_description& optionsDescription);

// Helper functions to add & get certain options in a standardized way
void addOptionHelp(boost::program_options::options_description& optionsDescription);
void addOptionRegisterAddress(boost::program_options::options_description& optionsDescription);
void addOptionRegisterValue(boost::program_options::options_description& optionsDescription);
void addOptionRegisterRange(boost::program_options::options_description& optionsDescription);
void addOptionChannel(boost::program_options::options_description& optionsDescription);
void addOptionSerialNumber(boost::program_options::options_description& optionsDescription);
void addOptionsChannelParameters(boost::program_options::options_description& optionsDescription);
int getOptionRegisterAddress(const boost::program_options::variables_map& variablesMap);
int getOptionRegisterValue(const boost::program_options::variables_map& variablesMap);
int getOptionChannel(const boost::program_options::variables_map& variablesMap);
int getOptionSerialNumber(const boost::program_options::variables_map& variablesMap);
int getOptionRegisterRange(const boost::program_options::variables_map& variablesMap);
ChannelParameters getOptionsChannelParameters(const boost::program_options::variables_map& variablesMap);
Parameters::Map getOptionsParameterMap(const boost::program_options::variables_map& variablesMap);

} // namespace Options
} // namespace Utilities
} // namespace Rorc
} // namespace AliceO2
