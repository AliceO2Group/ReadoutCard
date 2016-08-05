///
/// \file Program.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <boost/program_options.hpp>
#include "Utilities/Common.h"
#include "Utilities/Options.h"
#include "Utilities/UtilsDescription.h"
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {
namespace Utilities {

/// Helper class for making a RORC utility program. It handles:
/// - Creation of the options_descripotion object
/// - Creation of the variables_map object
/// - Help message
/// - Exceptions & error messages
/// - SIGINT signals
class Program
{
  public:

    Program();
    virtual ~Program();

    /// Execute the program using the given arguments
    int execute(int argc, char** argv);

  protected:

    /// Get the description of the program
    virtual UtilsDescription getDescription() = 0;

    /// Add the program's options
    virtual void addOptions(boost::program_options::options_description& optionsDescription) = 0;

    /// The main function of the program
    virtual void mainFunction(const boost::program_options::variables_map& variablesMap) = 0;

    /// Has the SIGINT signal been given? (usually Ctrl-C)
    bool isSigInt();

    /// Should output be verbose
    bool isVerbose();

    void printHelp (const boost::program_options::options_description& optionsDescription);

  private:

    bool verbose;
};

} // namespace Utilities
} // namespace Rorc
} // namespace AliceO2
