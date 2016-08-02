///
/// \file RorcUtilsCleanup.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that cleans up channel state
///

#include <iostream>
#include "ChannelUtilityFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include "RorcException.h"
#include <boost/filesystem/operations.hpp>

using namespace AliceO2::Rorc::Util;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

namespace {

const char* FORCE_SWITCH = "force";

template <typename ErrInfo, typename Exception, typename Vector>
void pushIfPresent(const Exception& exception, Vector& vector)
{
  if (auto info = boost::get_error_info<ErrInfo>(exception)) {
    vector.push_back(*info);
  }
}

class ProgramCleanup: public RorcUtilsProgram
{
  public:

    virtual ~ProgramCleanup()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Cleanup", "Cleans up RORC state", "./rorc-cleanup --serial=12345 --channel=0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionSerialNumber(options);
      Options::addOptionChannel(options);
      options.add_options()(FORCE_SWITCH,"Force cleanup of shared state files if normal cleanup fails");
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
    {
      auto serialNumber = Options::getOptionSerialNumber(map);
      auto channelNumber = Options::getOptionChannel(map);
      auto forceEnabled = bool(map.count(FORCE_SWITCH));

      try {
        // This non-forced cleanup asks the ChannelMaster to clean up itself.
        // It will not succeed if the channel was not initialized properly before the running of this program.
        cout << "### Attempting cleanup..." << endl;
        auto channel = ChannelUtilityFactory().getUtility(serialNumber, channelNumber);
        channel->utilityCleanupState();
        cout << "### Done!" << endl;
      }
      catch (std::exception& e) {
        cout << "### Cleanup failed!" << endl;

        if (forceEnabled) {
          // The forced cleanup will try to delete the files belonging to the channel
          if (isVerbose()) {
            cout << "Error: \n" << boost::diagnostic_information(e);
          }

          cout << "### Attempting forced cleanup..." << endl;

          // Try to get paths of the shared state files from the exception's error info
          std::vector<std::string> paths;
          pushIfPresent<errinfo_rorc_shared_lock_file>(e, paths);
          pushIfPresent<errinfo_rorc_shared_buffer_file>(e, paths);
          pushIfPresent<errinfo_rorc_shared_fifo_file>(e, paths);
          pushIfPresent<errinfo_rorc_shared_state_file>(e, paths);

          if (paths.empty()) {
            cout << "Failed to discover files to clean up\n";
            cout << "### Forced cleanup failed!" << endl;
            throw;
          }

          for (auto& p : paths) {
            cout << "Deleting file '" << p << "'\n";
            boost::filesystem::remove(p.c_str());
          }
          cout << "### Done!" << endl;
        }
        else {
          // Forced cleanup was not enabled, so rethrow the exception, which will abort the program
          throw;
        }
      }
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCleanup().execute(argc, argv);
}
