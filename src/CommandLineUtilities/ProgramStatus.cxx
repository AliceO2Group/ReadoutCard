// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramStatus.cxx
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
#include <boost/format.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramStatus : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Status", "Return current RoC configuration status",
             "roc-status --id 42:00.0\n" };
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
    auto params = Parameters::makeParameters(cardId, 2); //status available on BAR2
    // We care for all of the links
    //params.setLinkMask(Parameters::linkMaskFromString("0-23"));
    auto bar2 = ChannelFactory().getBar(params);

    CardType::type cardType = bar2->getCardType();
    if (cardType == CardType::type::Crorc) {
      std::cout << "CRORC status report not yet supported" << std::endl;
      return;
    } else if (cardType != CardType::type::Cru) {
      std::cout << "Invalid card type" << std::endl;
      return;
    }

    auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar2);

    Cru::ReportInfo reportInfo = cruBar2->report();

    std::ostringstream table;
    auto formatHeader = "  %-9s %-16s %-10s %-14s %-15s %-10s %-14s %-14s %-8s %-19s\n";
    auto formatRow = "  %-9s %-16s %-10s %-14s %-15s %-10s %-14.2f %-14.2f %-8s %-19.1f\n";
    auto header = (boost::format(formatHeader) % "Link ID" % "GBT Mode Tx/Rx" % "Loopback" % "GBT MUX" % "Datapath Mode" % "Datapath" % "RX freq(MHz)" % "TX freq(MHz)" % "Status" % "Optical power(uW)").str();
    auto lineFat = std::string(header.length(), '=') + '\n';
    auto lineThin = std::string(header.length(), '-') + '\n';

    if (mOptions.csvOut) {
      auto csvHeader = "Link ID,GBT Mode,Loopback,GBT Mux,Datapath Mode,Datapath,RX Freq(MHz),TX Freq(MHz),Status,Optical Power(uW)\n";
      std::cout << csvHeader;
    } else {
      table << lineFat << header << lineThin;
    }

    if (!mOptions.csvOut) {
      std::string clock = (reportInfo.ttcClock == 0 ? "TTC" : "Local");
      ;
      std::cout << "----------------------------" << std::endl;
      std::cout << clock << " clock | ";
      if (reportInfo.dynamicOffset) {
        std::cout << "Dynamic offset" << std::endl;
      } else {
        std::cout << "Fixed offset" << std::endl;
      }
      std::cout << "----------------------------" << std::endl;
    }

    for (const auto& el : reportInfo.linkMap) {
      auto link = el.second;
      int globalId = el.first; //Use the "new" link mapping
      std::string gbtTxMode = GbtMode::toString(link.gbtTxMode);
      std::string gbtRxMode = GbtMode::toString(link.gbtRxMode);
      std::string gbtTxRxMode = gbtTxMode + "/" + gbtRxMode;
      std::string loopback = (link.loopback == false ? "None" : "Enabled");

      std::string downstreamData;
      if (reportInfo.downstreamData == Cru::DATA_CTP) {
        downstreamData = "CTP";
      } else if (reportInfo.downstreamData == Cru::DATA_PATTERN) {
        downstreamData = "PATTERN";
      } else if (reportInfo.downstreamData == Cru::DATA_MIDTRG) {
        downstreamData = "MIDTRG";
      }

      std::string gbtMux = GbtMux::toString(link.gbtMux);
      if (gbtMux == "TTC") {
        gbtMux += ":" + downstreamData;
      }

      std::string datapathMode = DatapathMode::toString(link.datapathMode);

      std::string enabled = (link.enabled) ? "Enabled" : "Disabled";

      float rxFreq = link.rxFreq;
      float txFreq = link.txFreq;

      std::string linkStatus;
      if (link.stickyBit == Cru::LinkStatus::Up) {
        linkStatus = "UP";
      } else if (link.stickyBit == Cru::LinkStatus::UpWasDown) {
        linkStatus = "UP (was DOWN)";
      } else if (link.stickyBit == Cru::LinkStatus::Down) {
        linkStatus = "DOWN";
      }

      float opticalPower = link.opticalPower;

      if (mOptions.csvOut) {
        auto csvLine = std::to_string(globalId) + "," + gbtTxRxMode + "," + loopback + "," + gbtMux + "," + datapathMode + "," + enabled + "," +
                       std::to_string(rxFreq) + "," + std::to_string(txFreq) + "," + linkStatus + "," + std::to_string(opticalPower) + "\n";
        std::cout << csvLine;
      } else {
        auto format = boost::format(formatRow) % globalId % gbtTxRxMode % loopback % gbtMux % datapathMode % enabled % rxFreq % txFreq % linkStatus % opticalPower;
        table << format;
      }
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
  return ProgramStatus().execute(argc, argv);
}
