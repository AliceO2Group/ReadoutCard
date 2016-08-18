///
/// \file RorcUtilsSanityCheck.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that performs some basic sanity checks
///

#include <iostream>
#include <thread>
#include <chrono>
#include "Factory/ChannelUtilityFactory.h"
#include "RORC/CardType.h"
#include "RorcException.h"
#include <boost/format.hpp>
#include "Utilities/Program.h"

using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

namespace b = boost;

namespace {
class ProgramSanityCheck: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Sanity Check", "Does some basic sanity checks on the card",
          "./rorc-sanity-check --serial=12345 --channel=0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionSerialNumber(options);
      Options::addOptionChannel(options);
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      const int serialNumber = Options::getOptionSerialNumber(map);
      const int channelNumber = Options::getOptionChannel(map);

      cout << "Warning: if the RORC is in a bad state, this program may result in a crash and reboot of the host\n";
      cout << "  To proceed, type 'y'\n";
      cout << "  To abort, type anything else or give SIGINT (usually Ctrl-c)\n";
      int c = getchar();
      if (c != 'y' || isSigInt()) {
        return;
      }

      auto channel = AliceO2::Rorc::ChannelUtilityFactory().getUtility(serialNumber, channelNumber);
      channel->utilitySanityCheck(cout);
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramSanityCheck().execute(argc, argv);
}
