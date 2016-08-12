///
/// \file ProgramInitChannel.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that initializes a RORC channel
///

#include <Utilities/Common.h>
#include <Utilities/Options.h>
#include <Utilities/Program.h>
#include <iostream>
#include "RORC/ChannelFactory.h"
#include "RorcDevice.h"

using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

namespace {
class ProgramInitChannel: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Initialize Channel", "Initializes a RORC channel",
          "./rorc-init-channel --serial=12345 --channel=0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionSerialNumber(options);
      Options::addOptionChannel(options);
      Options::addOptionsChannelParameters(options);
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
    {
      auto serialNumber = Options::getOptionSerialNumber(map);
      auto channelNumber = Options::getOptionChannel(map);
      auto parameters = Options::getOptionsChannelParameters(map);
      auto cardsFound = RorcDevice::findSystemDevices();

      for (auto& card : cardsFound) {
        if (serialNumber == card.serialNumber) {
          cout << "Found card, initializing channel..." << endl;

          if (card.cardType == CardType::Cru) {
            if (parameters.dma.pageSize != 8l*1024l) {
              cout << "Warning: given page size != 8 kiB, required for CRU. Correcting automatically.\n";
              parameters.dma.pageSize = 8l*1024l;
            }
          }

          // Don't assign it to any value so it constructs and destructs completely before we say things are done
          ChannelFactory().getMaster(serialNumber, channelNumber, parameters);
          cout << "Done!" << endl;
          return;
        }
      }

      cout << "Could not find card with given serial number\n";
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramInitChannel().execute(argc, argv);
}
