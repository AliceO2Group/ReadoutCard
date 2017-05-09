/// \file ProgramListCards.cxx
/// \brief Utility that lists the RORC devices on the system
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include <sstream>
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "CommandLineUtilities/Common.h"
#include "RorcDevice.h"
#include "RORC/ChannelFactory.h"
#include <boost/format.hpp>

using namespace AliceO2::Rorc::CommandLineUtilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

namespace {
class ProgramListCards: public Program
{
  public:

    virtual Description getDescription()
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

      auto formatHeader = "  %-3s %-6s %-10s %-11s %-11s %-8s %-15s\n";
      auto formatRow = "  %-3s %-6s %-10s 0x%-9s 0x%-9s %-8s %-15s\n";
      auto header = boost::str(boost::format(formatHeader)
          % "#" % "Type" % "PCI Addr" % "Vendor ID" % "Device ID" % "Serial" % "FW Version");
      auto lineFat = std::string(header.length(), '=') + '\n';
      auto lineThin = std::string(header.length(), '-') + '\n';

      table << lineFat << header << lineThin;

      int i = 0;
      for (const auto& card : cardsFound) {
        // Try to figure out the firmware version
        std::string firmware = "n/a";
        try {
          Parameters params = Parameters::makeParameters(card.serialNumber, 0);
          // Temporary (hopefully) workaround, because ChannelMaster requires a buffer when initializing
          params.setBufferParameters(BufferParameters::File{"/dev/shm/rorc_channel_utility_dummy_buffer", 4*1024});
          firmware = ChannelFactory().getMaster(params)->getFirmwareInfo().value_or("n/a");
        }
        catch (const Exception& e) {
          if (isVerbose()) {
            cout << "Could not get firmware version string:\n" << boost::diagnostic_information(e) << '\n';
          }
        }

        table << boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciAddress.toString()
            % card.pciId.vendor % card.pciId.device % card.serialNumber % firmware;
        i++;
      }

      table << lineFat;
      cout << table.str();
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramListCards().execute(argc, argv);
}
