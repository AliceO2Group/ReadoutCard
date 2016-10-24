/// \file ProgramCleanup.cxx
/// \brief Utility that cleans up channel state
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include "Factory/ChannelUtilityFactory.h"
#include "RORC/Exception.h"
#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <Utilities/Common.h>
#include <Utilities/Options.h>
#include <Utilities/Program.h>

using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;
namespace po = boost::program_options;

namespace {

template <typename ErrInfo, typename Exception, typename Vector>
void pushIfPresent(const Exception& exception, Vector& vector)
{
  if (auto info = boost::get_error_info<ErrInfo>(exception)) {
    vector.push_back(*info);
  }
}

class ProgramCleanup: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return {"Cleanup", "Cleans up RORC state", "./rorc-cleanup --serial=12345 --channel=0"};
    }

    virtual void addOptions(po::options_description& options)
    {
      Options::addOptionSerialNumber(options);
      Options::addOptionChannel(options);
      options.add_options()("force",po::bool_switch(&mForceCleanup),
          "Force cleanup of shared state files if normal cleanup fails");
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      auto serialNumber = Options::getOptionSerialNumber(map);
      auto channelNumber = Options::getOptionChannel(map);

      try {
        // This non-forced cleanup asks the ChannelMaster to clean up itself.
        // It will not succeed if the channel was not initialized properly before the running of this program.
        cout << "### Attempting cleanup..." << endl;
        auto channel = ChannelUtilityFactory().getUtility(serialNumber, channelNumber);
        channel->utilityCleanupState();
        cout << "### Done!" << endl;
      }
      catch (const std::exception& e) {
        cout << "### Cleanup failed!" << endl;

        if (mForceCleanup) {
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
          auto namedMutex = boost::get_error_info<errinfo_rorc_named_mutex_name>(e);

          if (paths.empty() && !namedMutex) {
            cout << "Failed to discover files to clean up\n";
            cout << "### Forced cleanup failed!" << endl;
            throw;
          }

          for (const auto& p : paths) {
            cout << "Deleting file '" << p << "'\n";
            boost::filesystem::remove(p.c_str());
          }

          if (namedMutex) {
            cout << "Deleting named mutex '" << namedMutex->c_str() << "'\n";
            boost::interprocess::named_mutex::remove(namedMutex->c_str());
          }

          cout << "### Done!" << endl;
        }
        else {
          // Forced cleanup was not enabled, so rethrow the exception, which will abort the program
          cout << "(can rerun with --force)\n";
          throw;
        }
      }
    }

  private:

    bool mForceCleanup;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCleanup().execute(argc, argv);
}
