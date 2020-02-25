// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramMetrics.cxx
/// \brief Tool that returns current information about RoCs.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Cru/Constants.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "RocPciDevice.h"
#include "Utilities/Util.h"
#include <boost/format.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace pt = boost::property_tree;
namespace po = boost::program_options;

class ProgramMetrics : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Metrics", "Return current RoC parameters",
             "roc-metrics \n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()("json-out",
                          po::bool_switch(&mOptions.jsonOut),
                          "Toggle json-formatted output");
    options.add_options()("csv-out",
                          po::bool_switch(&mOptions.csvOut),
                          "Toggle csv-formatted output");
  }

  virtual void run(const boost::program_options::variables_map& /*map*/)
  {

    std::ostringstream table;
    auto formatHeader = "  %-3s %-6s %-10s %-10s %-19s %-20s %-19s %-26s\n";
    auto formatRow = "  %-3s %-6s %-10s %-10s %-19s %-20s %-19s %-26s\n";
    auto header = (boost::format(formatHeader) % "#" % "Type" % "PCI Addr" % "Temp (C)" % "#Dropped Packets" % "CTP Clock (MHz)" % "Local Clock (MHz)" % "Total Packets per second").str();
    auto lineFat = std::string(header.length(), '=') + '\n';
    auto lineThin = std::string(header.length(), '-') + '\n';

    if (mOptions.csvOut) {
      auto csvHeader = "#,Type,PCI Addr,Temp (C),#Dropped Packets,CTP Clock (MHz),Local Clock (MHz),Total Packets per second\n";
      std::cout << csvHeader;
    } else if (!mOptions.jsonOut) {
      table << lineFat << header << lineThin;
    }

    auto cardsFound = AliceO2::roc::RocPciDevice::findSystemDevices();

    pt::ptree root;
    int i = 0;
    for (const auto& card : cardsFound) {
      if (card.cardType == CardType::type::Crorc) {
        continue;
      }

      Parameters params0 = Parameters::makeParameters(card.pciAddress, 0);
      auto bar0 = ChannelFactory().getBar(params0);
      Parameters params2 = Parameters::makeParameters(card.pciAddress, 2);
      auto bar2 = ChannelFactory().getBar(params2);

      float temperature = bar2->getTemperature().value_or(0);
      int32_t dropped = bar2->getDroppedPackets(bar0->getEndpointNumber());
      float ctpClock = bar2->getCTPClock() / 1e6;
      float localClock = bar2->getLocalClock() / 1e6;
      uint32_t totalPacketsPerSecond = bar2->getTotalPacketsPerSecond(0);

      if (mOptions.jsonOut) {
        pt::ptree cardNode;

        // add kv pairs for this card
        cardNode.put("type", CardType::toString(card.cardType));
        cardNode.put("pciAddress", card.pciAddress.toString());
        cardNode.put("temperature", Utilities::toPreciseString(temperature));
        cardNode.put("droppedPackets", std::to_string(dropped));
        cardNode.put("ctpClock", Utilities::toPreciseString(ctpClock));
        cardNode.put("localClock", Utilities::toPreciseString(localClock));
        cardNode.put("totalPacketsPerSecond", std::to_string(totalPacketsPerSecond));

        // add the card node to the tree
        root.add_child(std::to_string(i), cardNode);
      } else if (mOptions.csvOut) {
        auto csvLine = std::to_string(i) + "," + CardType::toString(card.cardType) + "," + card.pciAddress.toString() + "," +
                       std::to_string(temperature) + "," + std::to_string(dropped) + "," + std::to_string(ctpClock) + "," +
                       std::to_string(localClock) + "," + std::to_string(totalPacketsPerSecond) + "\n";
        std::cout << csvLine;
      } else {
        auto format = boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciAddress.toString() % temperature % dropped % ctpClock % localClock % totalPacketsPerSecond;

        table << format;
      }
      i++;
    }

    if (mOptions.jsonOut) {
      pt::write_json(std::cout, root);
    } else if (!mOptions.csvOut) {
      auto lineFat = std::string(header.length(), '=') + '\n';
      table << lineFat;
      std::cout << table.str();
    }
  }

 private:
  struct OptionsStruct {
    bool jsonOut = false;
    bool csvOut = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramMetrics().execute(argc, argv);
}
