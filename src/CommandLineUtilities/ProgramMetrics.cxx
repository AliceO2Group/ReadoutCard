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
#include <boost/format.hpp>
#include <boost/optional/optional_io.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramMetrics : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Metrics", "Return current RoC parameters",
             "roc-metrics --id -1\n"
             "roc-metrics --id 42:00.0\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("csv-out",
                          po::bool_switch(&mOptions.csvOut),
                          "Toggle csv-formatted output");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    auto cardId = Options::getOptionCardId(map);
    std::vector<CardDescriptor> cardsFound;

    try {
      cardsFound.push_back(RocPciDevice(cardId).getCardDescriptor());
    } catch (boost::exception& e) {
      std::cout << boost::diagnostic_information(e);
      return;
    }

    std::ostringstream table;
    auto formatHeader = "  %-3s %-6s %-10s %-10s %-19s %-20s %-19s %-8s %-17s %-17s\n";
    auto formatRow = "  %-3s %-6s %-10s %-10s %-19s %-20s %-19s %-8s %-17s %-17s\n";
    auto header = (boost::format(formatHeader) % "#" % "Type" % "PCI Addr" % "Temp (C)" % "#Dropped Packets" % "CTP Clock (MHz)" % "Local Clock (MHz)" % "#links" % "#Wrapper 0 links" % "#Wrapper 1 links").str();
    auto lineFat = std::string(header.length(), '=') + '\n';
    auto lineThin = std::string(header.length(), '-') + '\n';

    if (mOptions.csvOut) {
      auto csvHeader = "#,Type,PCI Addr,Temp (C),#Dropped Packets,CTP Clock (MHz),Local Clock (MHz),#links,#Wrapper 0 links, #Wrapper 1 links\n";
      std::cout << csvHeader;
    } else {
      table << lineFat << header << lineThin;
    }

    int i = 0;
    for (const auto& card : cardsFound) {
      Parameters params0 = Parameters::makeParameters(card.pciAddress, 0);
      auto bar0 = ChannelFactory().getBar(params0);
      Parameters params2 = Parameters::makeParameters(card.pciAddress, 2);
      auto bar2 = ChannelFactory().getBar(params2);

      float temperature = bar2->getTemperature().value_or(0);
      int32_t dropped = bar2->getDroppedPackets(bar0->getEndpointNumber());
      float ctp_clock = bar2->getCTPClock() / 1e6;
      float local_clock = bar2->getLocalClock() / 1e6;
      int32_t links = bar2->getLinks();
      uint32_t links0 = bar2->getLinksPerWrapper(0);
      uint32_t links1 = bar2->getLinksPerWrapper(1);

      if (mOptions.csvOut) {
        auto csvLine = std::to_string(i) + "," + CardType::toString(card.cardType) + "," + card.pciAddress.toString() + "," + std::to_string(temperature) + "," + std::to_string(dropped) + "," + std::to_string(ctp_clock) + "," + std::to_string(local_clock) + "," + std::to_string(links) + "," + std::to_string(links0) + "," + std::to_string(links1) + "\n";
        std::cout << csvLine;
      } else {
        auto format = boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciAddress.toString() % temperature % dropped % ctp_clock % local_clock % links % links0 % links1;

        table << format;
      }
      i++;
    }

    if (!mOptions.csvOut) {
      auto lineFat = std::string(header.length(), '=') + '\n';
      table << lineFat;
      std::cout << table.str();
    }
  }

 private:
  struct OptionsStruct {
    bool csvOut = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramMetrics().execute(argc, argv);
}
