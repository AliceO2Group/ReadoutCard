/// \file Options.h
/// \brief Definition of functions for the ReadoutCard utilities to handle program options
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <boost/program_options.hpp>
#include <boost/exception/all.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ParameterTypes/GeneratorPattern.h"
#include "ReadoutCard/ParameterTypes/ResetLevel.h"

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Options {

// Helper functions to add & get certain options in a standardized way
void addOptionHelp(boost::program_options::options_description& optionsDescription);
void addOptionRegisterAddress(boost::program_options::options_description& optionsDescription);
void addOptionRegisterValue(boost::program_options::options_description& optionsDescription);
void addOptionRegisterRange(boost::program_options::options_description& optionsDescription);
void addOptionChannel(boost::program_options::options_description& optionsDescription);
void addOptionSerialNumber(boost::program_options::options_description& optionsDescription);
void addOptionResetLevel(boost::program_options::options_description& optionsDescription);
void addOptionCardId(boost::program_options::options_description& optionsDescription);
void addOptionsChannelParameters(boost::program_options::options_description& optionsDescription);
int getOptionRegisterAddress(const boost::program_options::variables_map& variablesMap);
int getOptionRegisterValue(const boost::program_options::variables_map& variablesMap);
int getOptionChannel(const boost::program_options::variables_map& variablesMap);
int getOptionSerialNumber(const boost::program_options::variables_map& variablesMap);
ResetLevel::type getOptionResetLevel(const boost::program_options::variables_map& variablesMap);
Parameters::CardIdType getOptionCardId(const boost::program_options::variables_map& variablesMap);
int getOptionRegisterRange(const boost::program_options::variables_map& variablesMap);
Parameters getOptionsParameterMap(const boost::program_options::variables_map& variablesMap);

} // namespace Options
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
