/// \file ProgramListCards.cxx
/// \brief Utility that lists the RORC devices on the system
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include <sstream>
#include "Utilities/Options.h"
#include "Utilities/Program.h"
#include "Utilities/Common.h"
#include "RorcDevice.h"
#include "Factory/ChannelUtilityFactory.h"
#include <boost/format.hpp>

using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

namespace {
class ProgramListCards: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return {"List Cards", "Lists installed RORC cards and some basic information about them", "./rorc-list-cards"};
    }

    virtual void addOptions(boost::program_options::options_description&)
    {
    }

    virtual void run(const boost::program_options::variables_map&)
    {
      auto cardsFound = AliceO2::Rorc::RorcDevice::findSystemDevices();

      std::ostringstream table;

      auto formatHeader = "  %-3s %-12s %-12s %-12s %-12s %-12s\n";
      auto formatRow = "  %-3s %-12s 0x%-10s 0x%-10s %-12s %-12s\n";
      auto header = boost::str(boost::format(formatHeader) % "#" % "Card Type" % "Vendor ID" % "Device ID" % "Serial Nr"
          % "FW Version");
      auto lineFat = std::string(header.length(), '=') + '\n';
      auto lineThin = std::string(header.length(), '-') + '\n';

      table << lineFat << header << lineThin;

      int i = 0;
      bool foundUninitialized = false;
      for (const auto& card : cardsFound) {
        // Try to figure out the firmware version
        std::string firmware = "n/a";
        try {
          Parameters params = Parameters::makeParameters(card.serialNumber, 0);
          auto firmwareVersion = ChannelUtilityFactory().getUtility(params)->utilityGetFirmwareVersion();
          firmware = std::to_string(firmwareVersion);
        }
        catch (const SharedStateException& e) {
          foundUninitialized = true;
          cout << e.what() << '\n';
        }

        table << boost::str(boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciId.vendor
            % card.pciId.device % card.serialNumber % firmware);
        i++;
      }

      table << lineFat;

      cout << "Found " << cardsFound.size() << " card(s)\n";

      if (foundUninitialized) {
        cout << "Found card(s) with invalid channel 0 shared state. Reading the firmware version from these is "
            "currently not supported by this utility\n";
      }

      cout << table.str();
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramListCards().execute(argc, argv);
}
