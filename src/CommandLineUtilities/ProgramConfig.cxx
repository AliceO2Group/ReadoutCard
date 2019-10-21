// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramConfig.cxx
/// \brief Tool that configures the CRU.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/CardConfigurator.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/FirmwareChecker.h"
#include "RocPciDevice.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramConfig : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Config", "Configure the CRU(s)",
             "roc-config --config-file file:roc.cfg\n"
             "roc-config --id 42:00.0 --links 0-23 --clock local --datapathmode packet --loopback --gbtmux ttc\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()("allow-rejection",
                          po::bool_switch(&mOptions.allowRejection),
                          "Flag to allow HBF rejection");
    options.add_options()("clock",
                          po::value<std::string>(&mOptions.clock)->default_value("LOCAL"),
                          "Clock [LOCAL, TTC]");
    options.add_options()("cru-id",
                          po::value<uint16_t>(&mOptions.cruId)->default_value(0x0),
                          "12-bit CRU ID");
    options.add_options()("datapathmode",
                          po::value<std::string>(&mOptions.datapathMode)->default_value("PACKET"),
                          "DatapathMode [PACKET, CONTINUOUS]");
    options.add_options()("downstreamdata",
                          po::value<std::string>(&mOptions.downstreamData)->default_value("CTP"),
                          "DownstreamData [CTP, PATTERN, MIDTRG]");
    options.add_options()("gbtmode",
                          po::value<std::string>(&mOptions.gbtMode)->default_value("GBT"),
                          "GBT MODE [GBT, WB]");
    options.add_options()("gbtmux",
                          po::value<std::string>(&mOptions.gbtMux)->default_value("TTC"),
                          "GBT MUX [TTC, DDG, SWT]");
    options.add_options()("links",
                          po::value<std::string>(&mOptions.links)->default_value("0"),
                          "Links to enable");
    options.add_options()("config-uri",
                          po::value<std::string>(&mOptions.configUri)->default_value(""),
                          "Configuration URI ('ini://[path]', 'json://[path]' or 'consul://[host][:port][/path]'");
    options.add_options()("loopback",
                          po::bool_switch(&mOptions.linkLoopbackEnabled),
                          "Flag to enable link loopback for DDG");
    options.add_options()("pon-upstream",
                          po::bool_switch(&mOptions.ponUpstreamEnabled),
                          "Flag to enable use of the PON upstream");
    options.add_options()("dyn-offset",
                          po::bool_switch(&mOptions.dynamicOffsetEnabled),
                          "Flag to enable the dynamic offset");
    options.add_options()("onu-address",
                          po::value<uint32_t>(&mOptions.onuAddress)->default_value(0x0),
                          "ONU address for PON upstream");
    options.add_options()("config-all",
                          po::bool_switch(&mOptions.configAll),
                          "Flag to configure all cards with default parameters on startup");
    options.add_options()("force-config",
                          po::bool_switch(&mOptions.forceConfig),
                          "Flag to force configuration and not check if the configuration is already present");
    options.add_options()("bypass-fw-check",
                          po::bool_switch(&mOptions.bypassFirmwareCheck),
                          "Flag to force configuration, bypassing the firmware checker");
    Options::addOptionCardId(options);
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    // Configure all cards found - Normally used during boot
    if (mOptions.configAll) {
      std::cout << "Running RoC Configuration for all cards" << std::endl;
      std::vector<CardDescriptor> cardsFound;
      if (mOptions.configUri == "") {
        std::cout << "A configuration URI is necessary with the startup-config flag set" << std::endl;
        return;
      }

      cardsFound = RocPciDevice::findSystemDevices();
      for (auto const& card : cardsFound) {
        std::cout << " __== " << card.pciAddress.toString() << " ==__ " << std::endl;
        auto params = Parameters::makeParameters(card.pciAddress, 2);
        if (!mOptions.bypassFirmwareCheck) {
          try {
            FirmwareChecker().checkFirmwareCompatibility(params);
            CardConfigurator(card.pciAddress, mOptions.configUri, mOptions.forceConfig);
          } catch (const Exception& e) {
            std::cout << boost::diagnostic_information(e) << std::endl;
          }
        }
      }
      return;
    }

    // Configure specific card
    auto cardId = Options::getOptionCardId(map);
    if (!mOptions.bypassFirmwareCheck) {
      try {
        FirmwareChecker().checkFirmwareCompatibility(cardId);
      } catch (const Exception& e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
        return;
      }
    }

    if (mOptions.configUri == "") {
      std::cout << "Configuring with command line arguments" << std::endl;
      auto params = Parameters::makeParameters(cardId, 2);
      params.setLinkMask(Parameters::linkMaskFromString(mOptions.links));
      params.setAllowRejection(mOptions.allowRejection);
      params.setClock(Clock::fromString(mOptions.clock));
      params.setCruId(mOptions.cruId);
      params.setDatapathMode(DatapathMode::fromString(mOptions.datapathMode));
      params.setDownstreamData(DownstreamData::fromString(mOptions.downstreamData));
      params.setGbtMode(GbtMode::fromString(mOptions.gbtMode));
      params.setGbtMux(GbtMux::fromString(mOptions.gbtMux));
      params.setLinkLoopbackEnabled(mOptions.linkLoopbackEnabled);
      params.setPonUpstreamEnabled(mOptions.ponUpstreamEnabled);
      params.setDynamicOffsetEnabled(mOptions.dynamicOffsetEnabled);
      params.setOnuAddress(mOptions.onuAddress);

      try {
        CardConfigurator(params, mOptions.forceConfig);
      } catch (const Exception& e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
      }
    } else {
      std::cout << "Configuring with config file" << std::endl;
      try {
        CardConfigurator(cardId, mOptions.configUri, mOptions.forceConfig);
      } catch (std::runtime_error e) {
        std::cout << "Error parsing the configuration..." << boost::diagnostic_information(e) << std::endl;
      }
    }
    return;
  }

  struct OptionsStruct {
    std::string clock = "local";
    std::string configUri = "";
    std::string datapathMode = "packet";
    std::string downstreamData = "Ctp";
    std::string gbtMode = "gbt";
    std::string gbtMux = "ttc";
    std::string links = "0";
    bool allowRejection = false;
    bool bypassFirmwareCheck = false;
    bool configAll = false;
    bool forceConfig = false;
    bool linkLoopbackEnabled = false;
    bool ponUpstreamEnabled = false;
    bool dynamicOffsetEnabled = false;
    uint32_t onuAddress = 0x0;
    uint16_t cruId = 0x0;
  } mOptions;

 private:
};

int main(int argc, char** argv)
{
  return ProgramConfig().execute(argc, argv);
}
