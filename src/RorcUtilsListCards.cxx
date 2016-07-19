///
/// \file RorcUtilsReadRegister.cxx
/// \author Pascal Boeschoten
///
/// Utility that lists the RORC devices on the system
///

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <boost/exception/diagnostic_information.hpp>
#include "RorcDevice.h"
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

int main(int, char**)
{
  auto optionsDescription = Options::createOptionsDescription();
  try {
    auto cardsFound = AliceO2::Rorc::RorcDevice::enumerateDevices();

    cout << "Found " << cardsFound.size() << " card(s)\n";

    auto formatHeader = "  %-3s %-12s %-12s %-12s %-12s \n";
    auto formatRow = "  %-3s %-12s 0x%-10s 0x%-10s %-12s \n";
    auto str = boost::str(boost::format(formatHeader) % "#" % "Card Type" % "Vendor ID" % "Device ID" % "Serial Nr.");
    auto line1 = std::string(str.length(), '=') + '\n';
    auto line2 = std::string(str.length(), '-') + '\n';

    cout << line1 << str << line2;

    int i = 0;
    for (auto& card : cardsFound) {
      auto cardType = AliceO2::Rorc::CardType::toString(card.cardType);
      cout << boost::str(boost::format(formatRow) % i % cardType % card.vendorId % card.deviceId % card.serialNumber);
      i++;
    }

    cout << line1;
  }
  catch (std::exception& e) {
    RORC_UTILS_HANDLE_EXCEPTION(e, DESCRIPTION, optionsDescription);
  }

  return 0;
}
