///
/// \file RorcUtilsListCards.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that lists the RORC devices on the system
///

#include <iostream>
#include <sstream>
#include "RorcDevice.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include "ChannelUtilityFactory.h"
#include <boost/format.hpp>

using namespace ::AliceO2::Rorc::Util;
using namespace ::AliceO2::Rorc;
using std::cout;
using std::endl;

namespace {
class ProgramListCards: public RorcUtilsProgram
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("List Cards", "Lists installed RORC cards and some basic information about them",
          "./rorc-list-cards");
    }

    virtual void addOptions(boost::program_options::options_description&)
    {
    }

    virtual void mainFunction(const boost::program_options::variables_map&)
    {
      auto cardsFound = AliceO2::Rorc::RorcDevice::enumerateDevices();

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
      for (auto& card : cardsFound) {
        // Try to figure out the firmware version
        std::string firmware = "unknown";
        try {
          auto firmwareVersion = ChannelUtilityFactory().getUtility(card.serialNumber, 0)->utilityGetFirmwareVersion();
          firmware = std::to_string(firmwareVersion);
        } catch (SharedStateException& e) {
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
