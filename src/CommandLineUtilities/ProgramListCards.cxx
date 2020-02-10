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
#include "ReadoutCard/FirmwareChecker.h"
#include "ReadoutCard/MemoryMappedFile.h"
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
namespace pt = boost::property_tree;
namespace po = boost::program_options;

namespace
{
class ProgramListCards : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "List Cards", "Lists installed cards and some basic information about them",
             "roc-list-cards\n"
             "roc-list-cards --json" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()("json-out",
                          po::bool_switch(&mOptions.jsonOut),
                          "Toggle json-formatted output");
  }

  virtual void run(const boost::program_options::variables_map&)
  {
    std::ostringstream table;

    auto formatHeader = "  %-3s %-6s %-10s %-8s %-13s %-5s %-11s %-11s %-25s %-17s\n";
    auto formatRow = "  %-3s %-6s %-10s %-8s %-13s %-5s 0x%-9s 0x%-9s %-25s %-17s\n";
    auto header = (boost::format(formatHeader) % "#" % "Type" % "PCI Addr" % "Serial" % "Endpoint ID" % "NUMA" % "Vendor ID" % "Device ID" % "FW Version" % "Card ID").str();
    auto lineFat = std::string(header.length(), '=') + '\n';
    auto lineThin = std::string(header.length(), '-') + '\n';

    if (!mOptions.jsonOut) {
      table << lineFat << header << lineThin;
    }

    // initialize ptree
    pt::ptree root;

    auto cardsFound = AliceO2::roc::RocPciDevice::findSystemDevices();

    int i = 0;
    for (const auto& card : cardsFound) {
      const std::string na = "n/a";
      std::string firmware = na;
      std::string cardId = na;
      std::string numaNode = std::to_string(card.numaNode);
      try {
        Parameters params2 = Parameters::makeParameters(card.pciAddress, 2);
        auto bar2 = ChannelFactory().getBar(params2);
        firmware = bar2->getFirmwareInfo().value_or(na);
        // Check if the firmware is tagged
        firmware = FirmwareChecker().resolveFirmwareTag(firmware);
        cardId = bar2->getCardId().value_or(na);
      } catch (const Exception& e) {
        if (isVerbose()) {
          std::cout << "Error parsing card information through BAR\n"
                    << boost::diagnostic_information(e) << '\n';
        }
      }

      std::string serial = std::to_string(card.serialId.getSerial());
      std::string endpoint = std::to_string(card.serialId.getEndpoint());

      if (!mOptions.jsonOut) {
        auto format = boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciAddress.toString() % serial % endpoint % card.numaNode % card.pciId.vendor % card.pciId.device %
                      firmware % cardId;

        table << format;
        std::cout << table.str();
        table.str("");
        table.clear();
      } else {
        pt::ptree cardNode;

        // add kv pairs for this card
        cardNode.put("type", CardType::toString(card.cardType));
        cardNode.put("pciAddress", card.pciAddress.toString());
        cardNode.put("serial", serial);
        cardNode.put("endpoint", endpoint);
        cardNode.put("numa", std::to_string(card.numaNode));
        cardNode.put("firmware", firmware);

        // add the card node to the tree
        root.add_child(std::to_string(i), cardNode);
      }

      // Update sequence number
      i++;
    }

    if (!mOptions.jsonOut && i) {
      table << lineFat;
      std::cout << table.str();
    } else {
      pt::write_json(std::cout, root);
    }
  }

 private:
  struct OptionsStruct {
    bool jsonOut = false;
  } mOptions;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramListCards().execute(argc, argv);
}
