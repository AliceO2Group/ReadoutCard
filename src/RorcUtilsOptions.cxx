///
/// \file RorcUtilsOptions.cxx
/// \author Pascal Boeschoten
///

#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsDescription.h"
#include <iostream>
#include <iomanip>
#include <bitset>

namespace AliceO2 {
namespace Rorc {
namespace Util {
namespace Options {

using std::cout;
using std::endl;
namespace po = boost::program_options;

po::options_description createOptionsDescription()
{
  po::options_description od ("Allowed options");
  addOptionHelp(od);
  return od;
}

po::variables_map getVariablesMap(int argc, char** argv, const po::options_description& od)
{
  po::variables_map variablesMap;
  po::store(po::parse_command_line(argc, argv, od), variablesMap);
  po::notify(variablesMap);

  if (variablesMap.count("help")) {
    throw std::runtime_error("Help");
  }

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


void addOptionHelp(po::options_description& od)
{
  od.add_options()("help,h", "Produce help message");
}

void addOptionChannel(po::options_description& od)
{
  od.add_options()("channel,c", po::value<int>()->default_value(0), "Channel");
}

void addOptionRegisterAddress(po::options_description& od)
{
  od.add_options()("address,a", po::value<std::string>(), "Register address in hex format");
}

void addOptionRegisterRange(po::options_description& od)
{
  od.add_options()("regrange,r", po::value<int>(), "Amount of registers to print past given address");
}

void addOptionSerialNumber(po::options_description& od)
{
  od.add_options()("serial,s", po::value<int>()->default_value(0), "Serial number");
}

template<typename T>
T getOption(std::string key, const po::variables_map& variablesMap)
{
  if (variablesMap.count(key) == 0){
    throw std::runtime_error("The option '" + key + "' is required but missing");
  }
  return variablesMap[key].as<T>();
}

int getOptionChannel(const po::variables_map& variablesMap)
{
  auto value = getOption<int>("channel", variablesMap);

  if (value < 0) {
    throw std::runtime_error("Channel negative");
  }

  return value;
}

int getOptionRegisterAddress(const po::variables_map& variablesMap)
{
  auto addressString = getOption<std::string>("address", variablesMap);

  std::stringstream ss;
  ss << std::hex << addressString;
  int address;
  ss >> address;

  if (address < 0 || address > 0xfff) {
    throw std::runtime_error("Address out of range, must be between 0 and 0xfff");
  }

  if (address % 4) {
    throw std::runtime_error("Address not a multiple of 4");
  }

  return address;
}

int getOptionRegisterRange(const po::variables_map& variablesMap)
{
  auto value = getOption<int>("regrange", variablesMap);

  if (value < 0) {
    throw std::runtime_error("Register range negative");
  }

  return value;
}

int getOptionSerialNumber(const po::variables_map& variablesMap)
{
  auto value = getOption<int>("serial", variablesMap);

  if (value < 0) {
    throw std::runtime_error("Serial number negative");
  }

  return value;
}

} // namespace Options
} // namespace Util
} // namespace Rorc
} // namespace AliceO2
