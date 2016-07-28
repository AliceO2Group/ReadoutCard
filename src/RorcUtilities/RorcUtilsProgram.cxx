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

/// Flag for SIGINT signal
std::atomic<bool> flagSigInt(false);

RorcUtilsProgram::RorcUtilsProgram()
    : verbose(false)
{
}

RorcUtilsProgram::~RorcUtilsProgram()
{
}

int RorcUtilsProgram::execute(int argc, char** argv)
{
  AliceO2::Rorc::Util::setSigIntHandler([](int){ flagSigInt = true; });

  auto optionsDescription = Options::createOptionsDescription();
  try {
    // We add a verbose switch
    optionsDescription.add_options()("verbose,v", "Verbose output");

    // Subclass will add own options
    addOptions(optionsDescription);

    // Parse options and get the resulting map of variables
    auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);

    if (variablesMap.count("help")) {
      AliceO2::Rorc::Util::Options::printHelp(getDescription(), optionsDescription);
      return 0;
    }

    if (variablesMap.count("verbose")) {
      verbose = true;
    }

    // Start the actual program
    mainFunction(variablesMap);
  }
  catch (boost::exception& boostException) {
    std::exception const* stdException = boost::current_exception_cast<std::exception const>();
    auto info = boost::get_error_info<AliceO2::Rorc::errinfo_rorc_generic_message>(boostException);

    if (info) {
      std::cout << "Error: " << *info << "\n\n";
    } else if (stdException) {
      std::cout << "Error: " << stdException->what() << "\n\n";
    } else {
      std::cout << "Error\n\n";
    }

    std::cout << '\n' << boost::diagnostic_information(boostException, verbose) << "\n";

    AliceO2::Rorc::Util::Options::printHelp(getDescription(), optionsDescription);
  }
  catch (std::exception& exception) {
    std::cout << "Error: " << exception.what() << "\n\n";
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
