// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramListCards.cxx
/// \brief Utility that lists the ReadoutCard devices on the system
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include <sstream>
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "RocPciDevice.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/MemoryMappedFile.h"
#include <boost/format.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using std::cout;
using std::endl;

namespace
{
class ProgramListCards : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "List Cards", "Lists installed cards and some basic information about them", "roc-list-cards" };
  }

  virtual void addOptions(boost::program_options::options_description&)
  {
  }

  virtual void run(const boost::program_options::variables_map&)
  {
    auto cardsFound = AliceO2::roc::RocPciDevice::findSystemDevices();

    std::ostringstream table;

    auto formatHeader = "  %-3s %-6s %-10s %-8s %-13s %-5s %-11s %-11s %-25s %-17s\n";
    auto formatRow = "  %-3s %-6s %-10s %-8s %-13s %-5s 0x%-9s 0x%-9s %-25s %-17s\n";
    auto header = (boost::format(formatHeader) % "#" % "Type" % "PCI Addr" % "Serial" % "Endpoint ID" % "NUMA" % "Vendor ID" % "Device ID" % "FW Version" % "Card ID").str();
    auto lineFat = std::string(header.length(), '=') + '\n';
    auto lineThin = std::string(header.length(), '-') + '\n';

    table << lineFat << header << lineThin;

    int i = 0;
    for (const auto& card : cardsFound) {
      const std::string na = "n/a";
      std::string firmware = na;
      std::string cardId = na;
      std::string numaNode = std::to_string(card.numaNode);
      int endpointNumber = -1;
      try {
        Parameters params0 = Parameters::makeParameters(card.pciAddress, 0);
        auto bar0 = ChannelFactory().getBar(params0);
        Parameters params2 = Parameters::makeParameters(card.pciAddress, 2);
        auto bar2 = ChannelFactory().getBar(params2);
        firmware = bar2->getFirmwareInfo().value_or(na);
        cardId = bar2->getCardId().value_or(na);
        endpointNumber = bar0->getEndpointNumber();
      } catch (const Exception& e) {
        if (isVerbose()) {
          cout << "Error parsing card information through BAR\n"
               << boost::diagnostic_information(e) << '\n';
        }
      }

      std::string serial;
      boost::optional<int32_t> serialCheck = card.serialNumber;
      if (serialCheck) {
        serial = std::to_string(serialCheck.get());
      } else {
        serial = "n/a";
      }

      auto format = boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciAddress.toString() % serial % endpointNumber % card.numaNode % card.pciId.vendor % card.pciId.device %
                    firmware % cardId;

      table << format;
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
