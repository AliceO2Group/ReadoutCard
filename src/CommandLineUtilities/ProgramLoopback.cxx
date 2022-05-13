
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
/// \file ProgramLoopback.cxx
/// \brief Tool that returns current configuration information about RoCs.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Cru/Common.h"
#include "Cru/Constants.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Utilities/Util.h"
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <thread>
#include <chrono>

using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
namespace pt = boost::property_tree;
namespace po = boost::program_options;

class ProgramLoopback : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Loopback", "Return GBT link loopback status",
             "o2-roc-loopback --id 1041:0\n"
             "o2-roc-loopback --id 1041:0 --pattern-mode=counter --counter-type=8bit\n"
             "o2-roc-loopback --id 1041:0 --skip-reset\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("interval",
                          po::value<int>(&mOptions.interval)->default_value(1),
                          "Print interval (in seconds)");
    options.add_options()("links",
                          po::value<std::string>(&mOptions.links)->default_value("0-11"),
                          "Links to show (all by default)");
    options.add_options()("skip-reset",
                          po::bool_switch(&mOptions.skipReset)->default_value(false),
                          "Skips resetting and initialization of the error counters");
    options.add_options()("pattern-mode",
                          po::value<std::string>(&mOptions.patternMode)->default_value("counter"),
                          "Pattern mode to use ('counter' or 'static')");
    options.add_options()("counter-type",
                          po::value<std::string>(&mOptions.counterType)->default_value("30bit"),
                          "Counter type ('30bit' or '8bit')");
    // Keep stats-mode as a placeholder in case some upside arises in the near future
    /*options.add_options()("stats-mode",
                          po::value<std::string>(&mOptions.statsMode)->default_value("all"),
                          "Stats mode ('count', 'fec' or 'all'");*/
    options.add_options()("high-mask",
                          po::value<std::string>(&mOptions.highMask)->default_value("0xffffffff"),
                          "High part of the mask");
    options.add_options()("med-mask",
                          po::value<std::string>(&mOptions.medMask)->default_value("0xffffffff"),
                          "Medium part of the mask");
    options.add_options()("low-mask",
                          po::value<std::string>(&mOptions.lowMask)->default_value("0xffffffff"),
                          "Low part of the mask");
    options.add_options()("expert-view",
                          po::bool_switch(&mOptions.expertView)->default_value(false),
                          "Enables expert view");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {
    std::ostringstream table;
    std::string formatHeader;
    std::string formatRow;
    std::string header;
    std::string lineFat;
    std::string lineThin;

    // initialize ptree
    pt::ptree root;

    auto cardId = Options::getOptionCardId(map);
    auto cardIdString = Options::getOptionCardIdString(map);
    auto card = RocPciDevice(cardId).getCardDescriptor();
    auto cardType = card.cardType;

    if (cardType == CardType::type::Crorc) {
      Logger::get() << "CRORC not supported" << LogErrorOps << endm;
    } else if (cardType == CardType::type::Cru) {
      if (mOptions.expertView) {
        formatHeader = "  %-9s %-10s %-19s %-16s %-12s %-21s %-17s\n";
        formatRow = "  %-9s %-10s %-19s %-16s %-12s %-21s %-17s\n";
        header = (boost::format(formatHeader) % "Link ID" % "PLL Lock" % "RX Locked to Data" % "Data layer UP" % "GBT PHY UP" % "RX Data Error Count" % "FEC Error Count").str();
      } else {
        formatHeader = "  %-9s %-12s %-21s %-17s\n";
        formatRow = "  %-9s %-12s %-21s %-17s\n";
        header = (boost::format(formatHeader) % "Link ID" % "GBT PHY UP" % "RX Data Error Count" % "FEC Error Count").str();
      }
      lineFat = std::string(header.length(), '=') + '\n';
      lineThin = std::string(header.length(), '-') + '\n';

      auto params = Parameters::makeParameters(cardId, 2); //status available on BAR2
      params.setLinkMask(Parameters::linkMaskFromString(mOptions.links));
      params.setGbtPatternMode(GbtPatternMode::fromString(mOptions.patternMode));
      params.setGbtCounterType(GbtCounterType::fromString(mOptions.counterType));
      //params.setGbtStatsMode(GbtStatsMode::fromString(mOptions.statsMode));
      params.setGbtHighMask(strtoul(mOptions.highMask.c_str(), NULL, 16));
      params.setGbtMedMask(strtoul(mOptions.medMask.c_str(), NULL, 16));
      params.setGbtLowMask(strtoul(mOptions.lowMask.c_str(), NULL, 16));
      auto bar2 = ChannelFactory().getBar(params);
      auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar2);

      if (!mOptions.skipReset) {
        auto loopbackStatsMap = cruBar2->getGbtLoopbackStats(true);
      }

      while (!isSigInt()) {
        table << lineFat << header << lineThin;
        auto loopbackStatsMap = cruBar2->getGbtLoopbackStats(false);

        /* PARAMETERS PER LINK */
        for (const auto& el : loopbackStatsMap) {
          int globalId = el.first; //Use the "new" link mapping
          auto stats = el.second;

          std::string pllLockString = Utilities::toBoolString(stats.pllLock);
          std::string rxLockedToDataString = Utilities::toBoolString(stats.rxLockedToData);
          std::string dataLayerUpString = Utilities::toBoolString(stats.dataLayerUp);
          std::string gbtPhyUpString = Utilities::toBoolString(stats.gbtPhyUp);

          auto format = boost::format(formatRow) % globalId;
          if (mOptions.expertView) {
            format = format %
                     pllLockString %
                     rxLockedToDataString %
                     dataLayerUpString;
          }
          format = format %
                   gbtPhyUpString %
                   stats.rxDataErrorCount %
                   stats.fecErrorCount;
          table << format;
        }
        // clear the screen
        // cleanest hack to have updateable multiline output without curses
        printf("\033[2J");
        printf("\033[%d;%dH", 0, 0);

        // print every second
        table << lineFat;
        std::cout << table.str() << std::flush;
        table.str("");
        std::this_thread::sleep_for(std::chrono::seconds(mOptions.interval));
      }
    } else {
      std::cout << "Invalid card type" << std::endl;
      return;
    }
  }

 private:
  struct OptionsStruct {
    std::string links = "0-11";
    std::string patternMode = "counter";
    std::string counterType = "30bit";
    //std::string statsMode = "all";
    std::string highMask = "0xffffffff";
    std::string medMask = "0xffffffff";
    std::string lowMask = "0xffffffff";
    bool skipReset = false;
    int interval = 1;
    bool expertView = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramLoopback().execute(argc, argv);
}
