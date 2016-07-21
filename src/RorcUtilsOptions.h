///
/// \file RorcUtilsOptions.h
/// \author Pascal Boeschoten
///
/// Functions for the RORC utilities to handle program options
///

#pragma once

#include <boost/program_options.hpp>
#include <boost/exception/all.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
#include "RorcUtilsDescription.h"
#include "RorcException.h"
#include "RORC/ChannelParameters.h"

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

void printErrorAndHelp(const std::exception& exception, const UtilsDescription& utilsDescription,
    const boost::program_options::options_description& optionsDescription);

/// Create an options_description object, with the help switch already added
boost::program_options::options_description createOptionsDescription();

/// Get the variables_map object from the program arguments.
/// Will throw an exception if the program arguments given are invalid or the help switch is given.
boost::program_options::variables_map getVariablesMap(int argc, char** argv,
    const boost::program_options::options_description& optionsDescription);

// Helper functions to add & get certain options in a standardized way
void addOptionHelp(boost::program_options::options_description& optionsDescription);
void addOptionRegisterAddress(boost::program_options::options_description& optionsDescription);
void addOptionRegisterRange(boost::program_options::options_description& optionsDescription);
void addOptionChannel(boost::program_options::options_description& optionsDescription);
void addOptionSerialNumber(boost::program_options::options_description& optionsDescription);
void addOptionsChannelParameters(boost::program_options::options_description& optionsDescription);
int getOptionRegisterAddress(const boost::program_options::variables_map& variablesMap);
int getOptionChannel(const boost::program_options::variables_map& variablesMap);
int getOptionSerialNumber(const boost::program_options::variables_map& variablesMap);
int getOptionRegisterRange(const boost::program_options::variables_map& variablesMap);
ChannelParameters getOptionsChannelParameters(const boost::program_options::variables_map& variablesMap);

#ifdef NDEBUG
/// Helper macro for handling exceptions in the utility programs
/// It's a macro because we want to preserve the exception context for the boost::current_exception function
/// The NDEBUG version omits the verbose diagnostic info
#define RORC_UTILS_HANDLE_EXCEPTION(_exception, _utilsDescription, _optionsDescription) do \
{ \
  boost::exception const* be = boost::current_exception_cast<boost::exception const>(); \
  if (be) { \
    if (auto info = boost::get_error_info<::AliceO2::Rorc::errinfo_rorc_generic_message>(_exception)) { \
      std::cout << "Error: " << *info << "\n\n"; \
    } else { \
      std::cout << "Error: " << _exception.what() << "\n\n"; \
    } \
    ::AliceO2::Rorc::Util::Options::printHelp(_utilsDescription, _optionsDescription); \
  } \
  else { \
    std::cout << "Error: " << _exception.what() << "\n\n"; \
  } \
} while (0)
#else
#define RORC_UTILS_HANDLE_EXCEPTION(_exception, _utilsDescription, _optionsDescription) do \
{ \
  boost::exception const* be = boost::current_exception_cast<boost::exception const>(); \
  if (be) { \
    if (auto info = boost::get_error_info<::AliceO2::Rorc::errinfo_rorc_generic_message>(_exception)) { \
      std::cout << "Error: " << *info << "\n\n"; \
      std::cout << "DEBUG INFO: " << boost::diagnostic_information(_exception) << "\n"; \
    } else { \
      std::cout << "Error: " << _exception.what() << "\n\n"; \
    } \
    ::AliceO2::Rorc::Util::Options::printHelp(_utilsDescription, _optionsDescription); \
  } \
  else { \
    std::cout << "Error: " << _exception.what() << "\n\n"; \
  } \
} while (0)
#endif

#define RORC_UTILS_HANDLE_HELP(_variablesMap, _utilDescription, _optDescription) \
if (_variablesMap.count("help")) { \
  ::AliceO2::Rorc::Util::Options::printHelp(_utilDescription, _optDescription); \
  return 0; \
}

} // namespace Options
} // namespace Util
} // namespace Rorc
} // namespace AliceO2
