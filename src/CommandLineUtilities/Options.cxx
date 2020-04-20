// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Options.cxx
/// \brief Implementation of functions for the ReadoutCard utilities to handle program options
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Options.h"
#include <iomanip>
#include <iostream>
#include <sys/ioctl.h>
#include <boost/algorithm/string.hpp>
#include "CommandLineUtilities/Common.h"
#include "ExceptionInternal.h"

namespace AliceO2
{
namespace roc
{
namespace CommandLineUtilities
{
namespace Options
{
namespace
{

using std::cout;
using std::endl;
namespace po = boost::program_options;
namespace b = boost;

/// Simple data holder struct that represents a program option
template <typename T>
struct Option {
  Option(std::string swtch, std::string description)
    : swtch(swtch), description(description)
  {
  }

  /// We need to split the swtch string on comma, because it might contain the short command line switch as well,
  /// but only the long switch (which comes first) is used for the variables_map
  std::string getLongSwitch() const
  {
    return swtch.substr(0, swtch.find(","));
  }

  /// The command line switch
  const std::string swtch;

  /// The description of the option
  const std::string description;
};

namespace option
{
// General options
static Option<int> channel("channel", "BAR channel number");
static Option<std::string> registerAddress("address", "Register address in hex format");
static Option<int> registerRange("range", "Amount of registers to print past given address");
static Option<std::string> registerValue("value", "Register value, either in decimal or hex (prefix with 0x)");
static Option<std::string> cardId("id", "Card ID: PCI Address, Serial ID, or sequence number, as reported by `roc-list-cards`");
static Option<std::string> resetLevel("reset", "Reset level [NOTHING, INTERNAL, INTERNAL_SIU]");
} // namespace option

/// Adds the given Option to the options_description
/// \param option The Option to add
/// \param options The options_description to add to
template <typename T>
void addOption(const Option<T>& option, po::options_description& options)
{
  options.add_options()(option.swtch.c_str(), po::value<T>(), option.description.c_str());
}

/// Get the value of the Option from the variables_map, throw if it can't be found
/// \param option The Option to retrieve
/// \param map The variables_map to retrieve from
template <typename T>
T getOption(const Option<T>& option, const po::variables_map& map)
{
  auto longSwitch = option.getLongSwitch();

  if (map.count(longSwitch) == 0) {
    BOOST_THROW_EXCEPTION(OptionRequiredException()
                          << ErrorInfo::Message("The option '" + option.swtch + "' is required but missing"));
  }

  po::variable_value v = map.at(longSwitch);
  return v.as<T>();
}
} // Anonymous namespace

// Add functions

void addOptionChannel(po::options_description& options)
{
  addOption(option::channel, options);
}

void addOptionCardId(po::options_description& options)
{
  addOption(option::cardId, options);
}

void addOptionRegisterAddress(po::options_description& options)
{
  addOption(option::registerAddress, options);
}

void addOptionRegisterValue(po::options_description& options)
{
  addOption(option::registerValue, options);
}

void addOptionRegisterRange(po::options_description& options)
{
  addOption(option::registerRange, options);
}

void addOptionResetLevel(po::options_description& options)
{
  addOption(option::resetLevel, options);
}

// Get functions

int getOptionChannel(const po::variables_map& map)
{
  auto value = getOption(option::channel, map);

  if (value < 0) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
                          << ErrorInfo::Message("Channel value is negative"));
  }

  return value;
}

uint32_t getOptionRegisterAddress(const po::variables_map& map)
{
  auto addressString = getOption<std::string>(option::registerAddress, map);

  std::stringstream ss;
  ss << std::hex << addressString;
  long address;
  ss >> address;

  if (address < 0) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
                          << ErrorInfo::Message("Address must be positive"));
  }

  uint32_t uaddress = (uint32_t)address;
  if (uaddress % 4) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
                          << ErrorInfo::Message("Address not a multiple of 4"));
  }

  return uaddress;
}

uint32_t getOptionRegisterValue(const po::variables_map& map)
{
  auto valueString = getOption<std::string>(option::registerValue, map);

  std::stringstream ss;
  if (valueString.find("0x") == 0) {
    ss << std::hex << valueString.substr(2);
  } else {
    ss << std::dec << valueString;
  }

  uint32_t value; // We need to read as unsigned, or it will detect an overflow if we input for example 0x80000000
  ss >> value;

  if (ss.fail()) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
                          << ErrorInfo::Message("Failed to parse 'register value' option"));
  }

  return value;
}

int getOptionRegisterRange(const po::variables_map& map)
{
  auto value = getOption(option::registerRange, map);

  if (value < 0) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
                          << ErrorInfo::Message("Register range negative"));
  }

  return value;
}

ResetLevel::type getOptionResetLevel(const po::variables_map& map)
{
  std::string string = getOption(option::resetLevel, map);
  try {
    return ResetLevel::fromString(string);
  } catch (const std::runtime_error& e) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
                          << ErrorInfo::Message("Failed to parse 'reset level' option"));
  }
}

Parameters::CardIdType getOptionCardId(const po::variables_map& map)
{
  std::string string = getOption(option::cardId, map);
  return Parameters::cardIdFromString(string);
}

std::string getOptionCardIdString(const po::variables_map& map)
{
  return getOption(option::cardId, map);
}

} // namespace Options
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
