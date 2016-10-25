/// \file Options.cxx
/// \brief Implementation of functions for the RORC utilities to handle program options
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <sys/ioctl.h>
#include <iostream>
#include <iomanip>
#include <bitset>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include "Utilities/Common.h"
#include "Utilities/Options.h"
#include "Utilities/UtilsDescription.h"
#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {
namespace Utilities {
namespace Options {

using std::cout;
using std::endl;
namespace po = boost::program_options;
namespace b = boost;

/// Simple data holder struct that represents a program option
template <typename T>
struct Option
{
    Option(std::string swtch, std::string description, bool hasDefault = false, T defaultValue = T())
        : swtch(swtch), description(description), hasDefault(hasDefault), defaultValue(defaultValue)
    {
    }

    /// The command line switch
    const std::string swtch;

    /// The description of the option
    const std::string description;

    /// Does the option have a default value?
    const bool hasDefault;

    /// The default value of the option, in case 'hasDefault' is true
    const T defaultValue;
};

/// We need to split the swtch string on comma, because it might contain the short command line switch as well,
/// but only the long switch (which comes first) is used for the variables_map
inline std::string getLongSwitch(const std::string& swtch)
{
  return swtch.substr(0, swtch.find(","));
}

namespace option {
// General options
static Option<int> channel("channel", "Card channel or BAR number");
static Option<std::string> registerAddress("address", "Register address in hex format");
static Option<int> registerRange("range", "Amount of registers to print past given address");
static Option<int> serialNumber("serial", "Card serial number");
static Option<std::string> registerValue("value", "Register value, either in decimal or hex (prefix with 0x)");

// Options for ChannelParameters
static Option<size_t> cpDmaPageSize("cp-dma-pagesize", "RORC page size in kibibytes", true, 4l);
static Option<size_t> cpDmaBufSize("cp-dma-bufmb", "DMA buffer size in mebibytes", true, 4);
static Option<bool> cpGenEnable("cp-gen-enable", "Enable data generator", true, true);
static Option<std::string> cpGenLoopback("cp-gen-loopb",
    "Loopback mode [NONE, RORC, DIU, SIU]", true, "RORC");
}

/// Adds the given Option to the options_description
/// \param option The Option to add
/// \param optionsDescription The options_description to add to
template <typename T>
void addOption(const Option<T>& option, po::options_description& optionsDescription)
{
  auto val = option.hasDefault ? po::value<T>()->default_value(option.defaultValue) : po::value<T>();
  optionsDescription.add_options()(option.swtch.c_str(), val, option.description.c_str());
}

/// Get the value of the Option from the variables_map, throw if it can't be found
/// \param option The Option to retrieve
/// \param variablesMap The variables_map to retrieve from
template <typename T>
T getOptionRequired(const Option<T>& option, const po::variables_map& variablesMap)
{
  auto switchLong = getLongSwitch(option.swtch);

  if (variablesMap.count(switchLong) == 0){
    BOOST_THROW_EXCEPTION(OptionRequiredException()
        << errinfo_rorc_error_message("The option '" + option.swtch + "' is required but missing"));
  }

  po::variable_value v = variablesMap.at(switchLong);
  return v.as<T>();
}

/// Get the value of the Option from the variables_map, if it is available
/// \param option The Option to retrieve
/// \param variablesMap The variables_map to retrieve from
/// \return The retrieved value, if it was found
template <typename T>
b::optional<T> getOptionOptional(const Option<T>& option, const po::variables_map& variablesMap)
{
  auto switchLong = getLongSwitch(option.swtch);

  if (variablesMap.count(switchLong) != 0){
    po::variable_value v = variablesMap.at(switchLong);
    return v.as<T>();
  } else {
    return b::none;
  }
}

po::options_description createOptionsDescription()
{
  // Get size of the terminal, the amount of columns is used for formatting the options
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  po::options_description optionsDescription("Allowed options", w.ws_col, w.ws_col/2);
  addOptionHelp(optionsDescription);
  return optionsDescription;
}

po::variables_map getVariablesMap(int argc, char** argv, const po::options_description& optionsDescription)
{
  po::variables_map variablesMap;
  try {
    po::store(po::parse_command_line(argc, argv, optionsDescription), variablesMap);
    po::notify(variablesMap);
  } catch (const po::unknown_option& e) {
    BOOST_THROW_EXCEPTION(ProgramOptionException()
        << errinfo_rorc_error_message("Unknown option '" + e.get_option_name() + "'"));
  }
  return variablesMap;
}

void addOptionHelp(po::options_description& optionsDescription)
{
  optionsDescription.add_options()("help", "Produce help message");
}

void addOptionChannel(po::options_description& optionsDescription)
{
  addOption(option::channel, optionsDescription);
}

void addOptionRegisterAddress(po::options_description& optionsDescription)
{
  addOption(option::registerAddress, optionsDescription);
}

void addOptionRegisterValue(po::options_description& optionsDescription)
{
  addOption(option::registerValue, optionsDescription);
}

void addOptionRegisterRange(po::options_description& optionsDescription)
{
  addOption(option::registerRange, optionsDescription);
}

void addOptionSerialNumber(po::options_description& optionsDescription)
{
  addOption(option::serialNumber, optionsDescription);
}

int getOptionChannel(const po::variables_map& variablesMap)
{
  auto value = getOptionRequired(option::channel, variablesMap);

  if (value < 0) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
        << errinfo_rorc_error_message("Channel value is negative"));
  }

  return value;
}

int getOptionRegisterAddress(const po::variables_map& variablesMap)
{
  auto addressString = getOptionRequired<std::string>(option::registerAddress, variablesMap);

  std::stringstream ss;
  ss << std::hex << addressString;
  int address;
  ss >> address;

  if (address < 0) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
        << errinfo_rorc_error_message("Address must be positive"));
  }

  if (address % 4) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
        << errinfo_rorc_error_message("Address not a multiple of 4"));
  }

  return address;
}

int getOptionRegisterValue(const po::variables_map& variablesMap)
{
  auto valueString = getOptionRequired<std::string>(option::registerValue, variablesMap);

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
        << errinfo_rorc_error_message("Failed to read register value option"));
  }

  return value;
}

int getOptionRegisterRange(const po::variables_map& variablesMap)
{
  auto value = getOptionRequired(option::registerRange, variablesMap);

  if (value < 0) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
        << errinfo_rorc_error_message("Register range negative"));
  }

  return value;
}

int getOptionSerialNumber(const po::variables_map& variablesMap)
{
  return getOptionRequired(option::serialNumber, variablesMap);
}

void addOptionsChannelParameters(po::options_description& optionsDescription)
{
  addOption(option::cpDmaPageSize, optionsDescription);
  addOption(option::cpDmaBufSize, optionsDescription);
  addOption(option::cpGenEnable, optionsDescription);
  addOption(option::cpGenLoopback, optionsDescription);
}

ChannelParameters getOptionsChannelParameters(const boost::program_options::variables_map& variablesMap)
{
  ChannelParameters cp;

  if (auto pageSizeKiB = getOptionOptional<size_t>(option::cpDmaPageSize, variablesMap)) {
    cp.dma.pageSize = pageSizeKiB.get() * 1024l;
  }

  if (auto bufSizeMiB = getOptionOptional<size_t>(option::cpDmaBufSize, variablesMap)) {
    cp.dma.bufferSize = bufSizeMiB.get() * 1024l * 1024l;
  }

  if (auto useGenerator = getOptionOptional<bool>(option::cpGenEnable, variablesMap)) {
    cp.generator.useDataGenerator = useGenerator.get();
  }

  if (auto loopbackString = getOptionOptional<std::string>(option::cpGenLoopback, variablesMap)) {
    if (!loopbackString.get().empty()) {
      try {
        auto loopback = LoopbackMode::fromString(loopbackString.get());
        cp.generator.loopbackMode = loopback;
      } catch (const std::out_of_range& e) {
        BOOST_THROW_EXCEPTION(InvalidOptionValueException()
            << errinfo_rorc_error_message("Invalid value for option '" + option::cpGenLoopback.swtch + "'"));
      }
    }
  }
  return cp;
}

Parameters::Map getOptionsParameterMap(const boost::program_options::variables_map& variablesMap)
{
  Parameters::Map map;
  auto cp = getOptionsChannelParameters(variablesMap);
  map.emplace("dma_page_size", std::to_string(cp.dma.pageSize));
  map.emplace("dma_buffer_size", std::to_string(cp.dma.bufferSize));
  map.emplace("generator_enabled", std::to_string(cp.generator.useDataGenerator));
  map.emplace("generator_loopback_mode", LoopbackMode::toString(cp.generator.loopbackMode));
  return map;
}

} // namespace Options
} // namespace Util
} // namespace Rorc
} // namespace AliceO2
