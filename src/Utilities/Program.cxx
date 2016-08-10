///
/// \file Program.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include <iostream>
#include <iomanip>
#include <atomic>
#include <boost/version.hpp>
#include "Utilities/Options.h"
#include "Utilities/Program.h"
#include "RORC/Version.h"
#include "Util.h"
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {
namespace Utilities {

using std::cout;
using std::endl;
namespace po = boost::program_options;

namespace {
  /// Flag for SIGINT signal
  std::atomic<bool> flagSigInt(false);

  void gotSigInt(int)
  {
    flagSigInt = true;
  }

  const char* HELP_SWITCH = "help";
  const char* VERBOSE_SWITCH = "verbose";
  const char* VERSION_SWITCH = "version";
}

Program::Program()
    : verbose(false)
{
}

Program::~Program()
{
}

void Program::printHelp (const po::options_description& optionsDescription)
{
  auto util = getDescription();
  cout << "#### RORC Utility: " << util.name << "\n"
  << util.description << '\n'
  << '\n'
  << optionsDescription
  << '\n'
  << "Example:\n"
  << "  " << util.usage << '\n';
}

int Program::execute(int argc, char** argv)
{
  Util::setSigIntHandler(gotSigInt);

  auto optionsDescription = Options::createOptionsDescription();
  auto prnHelp = [&](){ printHelp(optionsDescription); };

  // We add a verbose switch
  optionsDescription.add_options()(VERBOSE_SWITCH, "Verbose output");
  optionsDescription.add_options()(VERSION_SWITCH, "Display RORC library version");

  // Subclass will add own options
  addOptions(optionsDescription);

  try {
    // Parse options and get the resulting map of variables
    auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);

    if (variablesMap.count(HELP_SWITCH)) {
      prnHelp();
      return 0;
    }

    if (variablesMap.count(VERSION_SWITCH)) {
      cout << "RORC lib     " << Core::Version::getString() << '\n' << "VCS version  " << Core::Version::getRevision()
          << '\n';
      return 0;
    }

    verbose = bool(variablesMap.count(VERBOSE_SWITCH));

    // Start the actual program
    mainFunction(variablesMap);
  }
  catch (ProgramOptionException& e) {
    auto message = boost::get_error_info<AliceO2::Rorc::errinfo_rorc_error_message>(e);
    std::cout << "Program options invalid: " << *message << "\n\n";
    prnHelp();
  }
  catch (std::exception& e) {
#if (BOOST_VERSION >= 105400)
    std::cout << "Error:\n" << boost::diagnostic_information(e, isVerbose()) << "\n";
#else
#pragma message "BOOST_VERSION < 105400"
    std::cout << "Error:\n" << boost::diagnostic_information(e) << "\n";
#endif
  }

  return 0;
}

bool Program::isSigInt()
{
  return flagSigInt;
}

bool Program::isVerbose()
{
  return verbose;
}

} // namespace Utilities
} // namespace Rorc
} // namespace AliceO2
