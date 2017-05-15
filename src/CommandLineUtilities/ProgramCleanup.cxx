/// \file ProgramCleanup.cxx
/// \brief Utility that cleans up channel state
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using std::cout;
using std::endl;
namespace po = boost::program_options;

namespace {

class ProgramCleanup: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"Cleanup", "Cleans up ReadoutCard state", "roc-cleanup --id=12345 --channel=0"};
    }

    virtual void addOptions(po::options_description& options)
    {
      Options::addOptionCardId(options);
      Options::addOptionChannel(options);
      options.add_options()("force",po::bool_switch(&mForceCleanup),
          "Force cleanup of shared state files if normal cleanup fails");
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      auto cardId = Options::getOptionCardId(map);
      auto channelNumber = Options::getOptionChannel(map);

      // This non-forced cleanup asks the DmaChannel to clean up itself.
      // It will not succeed if the channel was not initialized properly before the running of this program.
      cout << "### Attempting cleanup...\n";
      auto params = AliceO2::roc::Parameters::makeParameters(cardId, channelNumber);
      params.setForcedUnlockEnabled(mForceCleanup);
      auto channel = ChannelFactory().getDmaChannel(params);
      cout << "### Done!\n";
    }

  private:
    bool mForceCleanup;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCleanup().execute(argc, argv);
}
