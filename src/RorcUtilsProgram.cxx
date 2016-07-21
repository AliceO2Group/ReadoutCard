///
/// \file RorcUtilsProgram.cxx
/// \author Pascal Boeschoten
///

#include "RorcUtilsProgram.h"
#include <iostream>
#include <iomanip>
#include <atomic>
#include "Util.h"
#include "RorcUtilsOptions.h"

namespace AliceO2 {
namespace Rorc {
namespace Util {

using std::cout;
using std::endl;
namespace po = boost::program_options;

/// Flag for SIGINT signal
std::atomic<bool> flagSigInt(false);

RorcUtilsProgram::RorcUtilsProgram()
{
}

RorcUtilsProgram::~RorcUtilsProgram()
{
}

int RorcUtilsProgram::execute(int argc, char** argv)
{
  AliceO2::Rorc::Util::setSigIntHandler([](int){ flagSigInt = true; });

  auto optionsDescription = Options::createOptionsDescription();
  addOptions(optionsDescription);
  auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);

  if (variablesMap.count("help")) {
    AliceO2::Rorc::Util::Options::printHelp(getDescription(), optionsDescription);
    return 0;
  }

  try {
    mainFunction(variablesMap);
  } catch (boost::exception& boostExceptione) {
    std::exception const* stdException = boost::current_exception_cast<std::exception const>();
    auto info = boost::get_error_info<AliceO2::Rorc::errinfo_rorc_generic_message>(boostExceptione);

    if (info) {
      std::cout << "Error: " << *info << "\n\n";
#ifndef NDEBUG
      std::cout << "DEBUG INFO: " << boost::diagnostic_information(boostExceptione) << "\n";
#endif
    } else if (stdException) {
      std::cout << "Error: " << stdException->what() << "\n\n";
    } else {
      std::cout << "Error\n\n";
    }
    AliceO2::Rorc::Util::Options::printHelp(getDescription(), optionsDescription);
  } catch (std::exception& exception) {
    std::cout << "Error: " << exception.what() << "\n\n";
  }

  return 0;
}

bool RorcUtilsProgram::isSigInt()
{
  return flagSigInt;
}

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
