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

using namespace AliceO2::Rorc::Util;
using std::cout;
using std::endl;

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
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
    {
      auto serialNumber = Options::getOptionSerialNumber(map);
      auto channelNumber = Options::getOptionChannel(map);
      auto channel = AliceO2::Rorc::ChannelUtilityFactory().getUtility(serialNumber, channelNumber);
      cout << "### Cleaning up..." << endl;
      channel->utilityCleanupState();
      cout << "### Done!" << endl;
    }
};

int main(int argc, char** argv)
{
  return ProgramCleanup().execute(argc, argv);
}
