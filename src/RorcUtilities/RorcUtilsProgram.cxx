///
/// \file RorcUtilsProgram.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "RorcUtilsProgram.h"
#include <iostream>
#include <iomanip>
#include <atomic>
#include "Util.h"
#include "RorcUtilsOptions.h"
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {
namespace Util {

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
}

RorcUtilsProgram::RorcUtilsProgram()
    : verbose(false)
{
}

RorcUtilsProgram::~RorcUtilsProgram()
{
}

int RorcUtilsProgram::execute(int argc, char** argv)
{
  Util::setSigIntHandler(gotSigInt);

  auto optionsDescription = Options::createOptionsDescription();
  auto printHelp = [&](){ Options::printHelp(getDescription(), optionsDescription); };

  // We add a verbose switch
  optionsDescription.add_options()("verbose,v", "Verbose output (usually only affects error output)");

  // Subclass will add own options
  addOptions(optionsDescription);

  try {
    // Parse options and get the resulting map of variables
    auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);

    if (variablesMap.count("help")) {
      printHelp();
      return 0;
    }

    if (variablesMap.count("verbose")) {
      verbose = true;
    }

    // Start the actual program
    mainFunction(variablesMap);
  }
  catch (ProgramOptionException& e) {
    auto message = boost::get_error_info<AliceO2::Rorc::errinfo_rorc_generic_message>(e);
    std::cout << "Program options invalid: " << *message << "\n\n";
    printHelp();
  }
  catch (boost::exception& e) {
    std::cout << "Error:\n" << boost::diagnostic_information(e/*, verbose*/) << "\n";
    printHelp();
  }
  catch (std::exception& e) {
    std::cout << "Error:\n" << e.what() << "\n\n";
    printHelp();
  }

  return 0;
}

bool RorcUtilsProgram::isSigInt()
{
  return flagSigInt;
}

bool RorcUtilsProgram::isVerbose()
{
  return verbose;
}

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
