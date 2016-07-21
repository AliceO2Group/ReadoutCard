///
/// \file RorcUtilsOptions.cxx
/// \author Pascal Boeschoten
///

#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsDescription.h"
#include <sys/ioctl.h>
#include <iostream>
#include <iomanip>
#include <bitset>
#include <boost/algorithm/string.hpp>
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {
namespace Util {
namespace Options {

using std::cout;
using std::endl;
namespace po = boost::program_options;

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
static Option<int> channel("channel,c", "Channel", true, 0);
static Option<std::string> registerAddress("address,a", "Register address in hex format");
static Option<int> registerRange("regrange,r", "Amount of registers to print past given address");
static Option<int> serialNumber("serial,s", "Serial number");

// Options for ChannelParameters
static Option<size_t> cpDmaPageSize("cp-dma-pagesize", "RORC page size in bytes", true, 4l * 1024l);
static Option<size_t> cpDmaBufSize("cp-dma-bufmb", "DMA buffer size in mebibytes", true, 4);
static Option<bool> cpGenEnable("cp-gen-enable", "Enable data generator", true, true);
static Option<std::string> cpGenLoopback("cp-gen-loopb",
    "Loopback mode [NONE, RORC, DIU, SIU]", true, "RORC");
}

/// Adds the given Option to the options_description
/// \param option The Option to add
/// \param od The options_description to add to
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
        << errinfo_rorc_generic_message("The option '" + option.swtch + "' is required but missing"));
  }

  po::variable_value v = variablesMap.at(switchLong);
  return v.as<T>();
}

/// Get the value of the Option from the variables_map, if it is available
/// \param option The Option to retrieve
/// \param variablesMap The variables_map to retrieve from
/// \param optionOut The retrieved value will be written here, if a value was found
template <typename T>
void getOptionOptional(const Option<T>& option, const po::variables_map& variablesMap, T& optionOut)
{
  auto switchLong = getLongSwitch(option.swtch);

  if (variablesMap.count(switchLong) != 0){
    po::variable_value v = variablesMap.at(switchLong);
    optionOut = v.as<T>();
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
  po::store(po::parse_command_line(argc, argv, optionsDescription), variablesMap);
  po::notify(variablesMap);
  return variablesMap;
}

void printHelp (const UtilsDescription& util, const po::options_description& optionsDescription)
{
  cout << "Rorc Utils - " << util.name << "\n"
  << "  " << util.description << "\n"
  << "\n"
  << optionsDescription
  << "\n"
  << "Example:\n"
  << "  " << util.usage << endl;
}

void printErrorAndHelp(const std::string& errorMessage, const UtilsDescription& utilsDescription,
    const boost::program_options::options_description& optionsDescription)
{
  cout << errorMessage << "\n\n";
  printHelp(utilsDescription, optionsDescription);
}

void addOptionHelp(po::options_description& optionsDescription)
{
  optionsDescription.add_options()("help,h", "Produce help message");
}

void addOptionChannel(po::options_description& optionsDescription)
{
  addOption(option::channel, optionsDescription);
}

void addOptionRegisterAddress(po::options_description& optionsDescription)
{
  addOption(option::registerAddress, optionsDescription);
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
        << errinfo_rorc_generic_message("Channel value is negative"));
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

  if (address < 0 || address > 0xfff) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
        << errinfo_rorc_generic_message("Address out of range, must be between 0 and 0xfff"));
  }

  if (address % 4) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
        << errinfo_rorc_generic_message("Address not a multiple of 4"));
  }

  return address;
}

int getOptionRegisterRange(const po::variables_map& variablesMap)
{
  auto value = getOptionRequired(option::registerRange, variablesMap);

  if (value < 0) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
            << errinfo_rorc_generic_message("Register range negative"));
  }

  return value;
}

int getOptionSerialNumber(const po::variables_map& variablesMap)
{
  auto value = getOptionRequired(option::serialNumber, variablesMap);

  if (value < 0) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
            << errinfo_rorc_generic_message("Serial number negative"));
  }

  return value;
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
  getOptionOptional(option::cpDmaPageSize, variablesMap, cp.dma.pageSize);

  size_t bufSizeMiB;
  getOptionOptional(option::cpDmaBufSize, variablesMap, bufSizeMiB);
  cp.dma.bufferSize = bufSizeMiB * 1024l * 1024l;

  getOptionOptional(option::cpGenEnable, variablesMap, cp.generator.useDataGenerator);

  auto loopbackString = std::string();
  getOptionOptional(option::cpGenLoopback, variablesMap, loopbackString);
  if (!loopbackString.empty()) {
    try {
      auto loopback = LoopbackMode::fromString(loopbackString);
      cp.generator.loopbackMode = loopback;
    } catch (std::out_of_range& e) {
      throw std::runtime_error("Invalid value for option '" + option::cpGenLoopback.swtch + "'");
    }
  }
  return cp;
}

} // namespace Options
} // namespace Util
} // namespace Rorc
} // namespace AliceO2
