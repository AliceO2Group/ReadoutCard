
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file ProgramTriggerMonitor.cxx
/// \brief Tool that returns monitoring information about LTU triggers.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Cru/CruBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
namespace pt = boost::property_tree;
namespace po = boost::program_options;

class ProgramTriggerMonitor : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Trigger Monitor", "Return LTU trigger monitoring information",
             "o2-roc-trig-monitor --id 42:00.0\n"
             "o2-roc-trig-monitor --id 42:00.0 --force # for pre-production CRUs\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("updateable",
                          po::bool_switch(&mOptions.updateable),
                          "Toggle updateable output");
    options.add_options()("force-report",
                          po::bool_switch(&mOptions.forceReport),
                          "Force report for invalid serial numbers");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    auto cardId = Options::getOptionCardId(map);
    Parameters params;
    std::shared_ptr<BarInterface> bar;
    auto card = RocPciDevice(cardId).getCardDescriptor();
    auto cardType = card.cardType;

    if (!mOptions.forceReport && (card.serialId.getSerial() == 0x7fffffff || card.serialId.getSerial() == 0x0)) {
      std::cout << "Bad serial reported, bad card state, exiting" << std::endl;
      return;
    }

    if (cardType == CardType::type::Crorc) {
      std::cout << "Only CRU supported, exiting" << std::endl;
      return;
    } else if (cardType == CardType::type::Cru) {

      params = Parameters::makeParameters(cardId, 2); //status available on BAR2
      bar = ChannelFactory().getBar(params);
      auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar);

      std::ostringstream table;
      auto formatHeader = "  %-12s %-15s %-12s %-15s %-12s %-15s %-12s %-12s\n";
      auto formatRow = "  %-12s %-15.3f %-12s %-15.3f %-12s %-15.3f %-12s %-12s\n";
      auto formatRowUpdateable = "  %-12s %-15.3f %-12s %-15.3f %-12s %-15.3f %-12s %-12s";
      auto header = (boost::format(formatHeader) % "HB" % "HB rate (kHz)" % "PHY" % "PHY rate (kHz)" % "TOF" % "TOF rate (kHz)" % "SOX" % "EOX").str();
      auto lineFat = std::string(header.length(), '=') + '\n';
      auto lineThin = std::string(header.length(), '-') + '\n';

      table << lineFat << header << lineThin;

      // initialize ptrees
      pt::ptree root;
      pt::ptree gbtLinks;

      if (mOptions.updateable) {
        std::cout << table.str(); // header

        while (!isSigInt()) {
          Cru::TriggerMonitoringInfo tmi = cruBar2->monitorTriggers(true);
          auto format = boost::format(formatRowUpdateable) % tmi.hbCount % tmi.hbRate % tmi.phyCount % tmi.phyRate % tmi.tofCount % tmi.tofRate % tmi.soxCount % tmi.eoxCount;
          std::cout << '\r' << format << std::flush;
        }

        std::cout << std::endl
                  << lineFat;
      } else {
        Cru::TriggerMonitoringInfo tmi = cruBar2->monitorTriggers();
        auto format = boost::format(formatRow) % tmi.hbCount % tmi.hbRate % tmi.phyCount % tmi.phyRate % tmi.tofCount % tmi.tofRate % tmi.soxCount % tmi.eoxCount;
        table << format << lineFat;
        std::cout << table.str();
      }

    } else if (cardType != CardType::type::Cru) {
      std::cout << "Invalid card type" << std::endl;
      return;
    }
  }

 private:
  struct OptionsStruct {
    bool updateable = false;
    bool forceReport = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramTriggerMonitor().execute(argc, argv);
}
