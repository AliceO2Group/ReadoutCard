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

#include <Monitoring/MonitoringFactory.h>
using namespace o2::monitoring;

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
namespace pt = boost::property_tree;
namespace po = boost::program_options;

class ProgramMetrics : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Metrics", "Return current RoC parameters",
             "roc-metrics \n"
             "roc-metrics --json \n"
             "roc-metrics --monitoring\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()("json-out",
                          po::bool_switch(&mOptions.jsonOut),
                          "Toggle json-formatted output");
    options.add_options()("monitoring",
                          po::bool_switch(&mOptions.monitoring),
                          "Toggle monitoring metrics sending");
  }

  virtual void run(const boost::program_options::variables_map& /*map*/)
  {

    std::ostringstream table;
    auto formatHeader = "  %-3s %-6s %-10s %-8s %-10s %-10s %-19s %-20s %-19s %-26s\n";
    auto formatRow = "  %-3s %-6s %-10s %-8s %-10s %-10s %-19s %-20s %-19s %-26s\n";
    auto header = (boost::format(formatHeader) % "#" % "Type" % "PCI Addr" % "Serial" % "Endpoint" % "Temp (C)" % "#Dropped Packets" % "CTP Clock (MHz)" % "Local Clock (MHz)" % "Total Packets per second").str();
    auto lineFat = std::string(header.length(), '=') + '\n';
    auto lineThin = std::string(header.length(), '-') + '\n';

    if (!mOptions.jsonOut) {
      table << lineFat << header << lineThin;
    }

    auto cardsFound = AliceO2::roc::RocPciDevice::findSystemDevices();

    // Monitoring instance to send metrics
    std::unique_ptr<Monitoring> monitoring;
    if (mOptions.monitoring) {
      monitoring = MonitoringFactory::Get(getMonitoringUri());
    }

    // Used for the JSON output
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
      uint32_t totalPacketsPerSecond = bar2->getTotalPacketsPerSecond(bar0->getEndpointNumber());

      if (card.serialId.getSerial() == 0x7fffffff || card.serialId.getSerial() == 0x0) {
        std::cout << "Bad serial reported, bad card state, exiting" << std::endl;
        return;
      }

      if (mOptions.monitoring) {
        monitoring->send(Metric{ "card" }
                           .addValue(card.pciAddress.toString(), "pciAddress")
                           .addValue(temperature, "temperature")
                           .addValue(dropped, "droppedPackets")
                           .addValue(ctpClock, "ctpClock")
                           .addValue(localClock, "localClock")
                           .addValue((int)totalPacketsPerSecond, "totalPacketsPerSecond")
                           .addTag(tags::Key::SerialId, card.serialId.getSerial())
                           .addTag(tags::Key::Endpoint, card.serialId.getEndpoint())
                           .addTag(tags::Key::ID, card.sequenceId)
                           .addTag(tags::Key::Type, tags::Value::CRU));

      } else if (mOptions.jsonOut) {
        pt::ptree cardNode;

        // add kv pairs for this card
        cardNode.put("type", CardType::toString(card.cardType));
        cardNode.put("pciAddress", card.pciAddress.toString());
        cardNode.put("serial", card.serialId.getSerial());
        cardNode.put("endpoint", card.serialId.getEndpoint());
        cardNode.put("temperature", Utilities::toPreciseString(temperature));
        cardNode.put("droppedPackets", std::to_string(dropped));
        cardNode.put("ctpClock", Utilities::toPreciseString(ctpClock));
        cardNode.put("localClock", Utilities::toPreciseString(localClock));
        cardNode.put("totalPacketsPerSecond", std::to_string(totalPacketsPerSecond));

        // add the card node to the tree
        root.add_child(std::to_string(i), cardNode);
      } else {
        auto format = boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciAddress.toString() % card.serialId.getSerial() % card.serialId.getEndpoint() % temperature % dropped % ctpClock % localClock % totalPacketsPerSecond;

        table << format;
      }
      i++;
    }

    if (mOptions.jsonOut) {
      pt::write_json(std::cout, root);
    } else if (!mOptions.monitoring) {
      auto lineFat = std::string(header.length(), '=') + '\n';
      table << lineFat;
      std::cout << table.str();
    }
  }

 private:
  struct OptionsStruct {
    bool jsonOut = false;
    bool monitoring = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramMetrics().execute(argc, argv);
}
