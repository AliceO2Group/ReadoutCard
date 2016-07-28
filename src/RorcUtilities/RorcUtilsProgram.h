///
/// \file RorcUtilsProgram.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <boost/program_options.hpp>
#include "RorcUtilsDescription.h"

namespace AliceO2 {
namespace Rorc {
namespace Util {

/// Helper class for making a RORC utility program. It handles:
/// - Creation of the options_descripotion object
/// - Creation of the variables_map object
/// - Help message
/// - Exceptions & error messages
/// - SIGINT signals
class RorcUtilsProgram
{
  public:

    RorcUtilsProgram();
    virtual ~RorcUtilsProgram();

    /// Execute the program using the given arguments
    int execute(int argc, char** argv);

  protected:

    /// Get the description of the program
    virtual UtilsDescription getDescription() = 0;

    /// Add the program's options
    virtual void addOptions(boost::program_options::options_description& optionsDescription) = 0;

    /// The main function of the program
    virtual void mainFunction(boost::program_options::variables_map& variablesMap) = 0;

    /// Has the SIGINT signal been given? (usually Ctrl-C)
    bool isSigInt();

    /// Should output be verbose
    bool isVerbose();

  private:

    bool verbose;
};

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
