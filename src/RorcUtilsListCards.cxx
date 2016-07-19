///
/// \file RorcUtilsReadRegister.cxx
/// \author Pascal Boeschoten
///
/// Utility that reads a register from a RORC
///

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/ChannelFactory.h"
#include "RORC/CardType.h"
#include "RorcDeviceEnumerator.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsDescription.h"
#include <boost/format.hpp>

using namespace AliceO2::Rorc::Util;
using std::cout;
using std::endl;

static const UtilsDescription DESCRIPTION(
    "List Cards",
    "Lists installed RORC cards",
    "./rorc-list-cards"
    );

int main(int argc, char** argv)
{
  auto optionsDescription = Options::createOptionsDescription();
  try {
    AliceO2::Rorc::RorcDeviceEnumerator enumerator;
    auto cardsFound = enumerator.getCardsFound();

    cout << "Found " << cardsFound.size() << " card(s)\n";

    auto formatString = "  %-12s %-12s %-12s %-12d \n";
    auto str = boost::str(boost::format(formatString) % "Card Type" % "Device ID" % "Vendor ID" % "Serial Nr.");
    auto line1 = std::string(str.length(), '=') + '\n';
    auto line2 = std::string(str.length(), '-') + '\n';

    cout << line1 << str << line2;

    for (auto& card : cardsFound) {
      auto cardType = AliceO2::Rorc::CardType::toString(card.cardType);
      cout << boost::str(boost::format(formatString) % cardType % card.deviceId % card.vendorId % card.serialNumber);
    }

    cout << line1;
  }
  catch (std::exception& e) {
    RORC_UTILS_HANDLE_EXCEPTION(e, DESCRIPTION, optionsDescription);
  }

  return 0;
}
