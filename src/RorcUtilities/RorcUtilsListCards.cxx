///
/// \file RorcUtilsListCards.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that lists the RORC devices on the system
///

#include <iostream>
#include "RorcDevice.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include <boost/format.hpp>

using namespace AliceO2::Rorc::Util;
using std::cout;
using std::endl;

namespace {
class ProgramListCards: public RorcUtilsProgram
{
  public:
    virtual ~ProgramListCards()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("List Cards", "Lists installed RORC cards", "./rorc-list-cards");
    }

    virtual void addOptions(boost::program_options::options_description&)
    {
    }

    virtual void mainFunction(const boost::program_options::variables_map&)
    {
      auto cardsFound = AliceO2::Rorc::RorcDevice::enumerateDevices();

      cout << "Found " << cardsFound.size() << " card(s)\n";

      auto formatHeader = "  %-3s %-12s %-12s %-12s %-12s \n";
      auto formatRow = "  %-3s %-12s 0x%-10s 0x%-10s %-12s \n";
      auto header = boost::str(boost::format(formatHeader) % "#" % "Card Type" % "Vendor ID" % "Device ID" % "Serial Nr.");
      auto lineFat = std::string(header.length(), '=') + '\n';
      auto lineThin = std::string(header.length(), '-') + '\n';

      cout << lineFat << header << lineThin;

      int i = 0;
      for (auto& card : cardsFound) {
        auto cardType = AliceO2::Rorc::CardType::toString(card.cardType);
        cout << boost::str(boost::format(formatRow) % i % cardType % card.pciId.vendor % card.pciId.device % card.serialNumber);
        i++;
      }

      cout << lineFat;
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramListCards().execute(argc, argv);
}
