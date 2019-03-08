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
#include "RocPciDevice.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramConfig: public Program
{
  public:

  virtual Description getDescription()
  {
    return {"Config", "Configure the CRU(s)", 
      "roc-config --config-file file:roc.cfg\n"
      "roc-config --id 42:00.0 --links 0-23 --clock local --gbtmode packet --loopback INTERNAL -gbtmux ttc\n"};
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()
      ("clock",
       po::value<std::string>(&mOptions.clock)->default_value("LOCAL"),
       "Clock [LOCAL, TTC]")
      ("datapathmode",
       po::value<std::string>(&mOptions.datapathMode)->default_value("PACKET"),
       "DatapathMode [PACKET, CONTINUOUS]")
      ("downstreamdata",
       po::value<std::string>(&mOptions.downstreamData)->default_value("CTP"),
       "DownstreamData [CTP, PATTERN, MIDTRG]")
      ("gbtmode",
       po::value<std::string>(&mOptions.gbtMode)->default_value("GBT"),
       "GBT MODE [GBT, WB]")
      ("gbtmux",
       po::value<std::string>(&mOptions.gbtMux)->default_value("TTC"),
       "GBT MUX [TTC, DDG, SC]")
      ("links", 
       po::value<std::string>(&mOptions.links)->default_value("0"),
       "Links to enable")
      ("config-file",
       po::value<std::string>(&mOptions.configFile)->default_value(""),
       "Configuration file [file:*.cfg]")
      ("loopback",
       po::bool_switch(&mOptions.linkLoopbackEnabled),
       "Flag to enable link loopback for DDG")
      ("config-all",
       po::bool_switch(&mOptions.configAll),
       "Flag to configure all cards with default parameters on startup")
      ("force-config",
       po::bool_switch(&mOptions.forceConfig),
       "Flag to force configuration and not check if the configuration is already present");
    Options::addOptionCardId(options);
  }

  virtual void run(const boost::program_options::variables_map& map) 
  {

    // Configure all cards found - Normally used during boot
    if (mOptions.configAll) {
      std::cout << "Running RoC Configuration for all cards" << std::endl;
      std::vector<CardDescriptor> cardsFound;
      if(mOptions.configFile == "") {
        std::cout << "A configuration file is necessary with the startup-config flag set" << std::endl;
        return;
      }

      cardsFound = RocPciDevice::findSystemDevices();
      for(auto const& card: cardsFound) {
        std::cout << " __== " << card.pciAddress.toString() << " ==__ " << std::endl;
        auto params = Parameters::makeParameters(card.pciAddress, 2);
        try {
          auto cardConfigurator = CardConfigurator(card.pciAddress, mOptions.configFile, mOptions.forceConfig);
        } catch(...) {
          std::cout << "Something went badly reading the configuration file..." << std::endl;
        }
      }
      return;
    }

    // Configure specific card
    auto cardId = Options::getOptionCardId(map);

    if (mOptions.configFile == "") {
      std::cout << "Configuring with command line arguments" << std::endl;
      auto params = Parameters::makeParameters(cardId, 2);
      params.setLinkMask(Parameters::linkMaskFromString(mOptions.links));
      params.setClock(Clock::fromString(mOptions.clock));
      params.setDatapathMode(DatapathMode::fromString(mOptions.datapathMode));
      params.setDownstreamData(DownstreamData::fromString(mOptions.downstreamData));
      params.setGbtMode(GbtMode::fromString(mOptions.gbtMode));
      params.setGbtMux(GbtMux::fromString(mOptions.gbtMux));
      params.setLinkLoopbackEnabled(mOptions.linkLoopbackEnabled);

      auto cardConfigurator = CardConfigurator(params, mOptions.forceConfig);
    } else if (!strncmp(mOptions.configFile.c_str(), "file:", 5)) {
      std::cout << "Configuring with config file" << std::endl;
      try {
        auto cardConfigurator = CardConfigurator(cardId, mOptions.configFile, mOptions.forceConfig);
      } catch(std::runtime_error e) {
        std::cout << "Something went badly reading the configuration file..." << e.what() << std::endl;
      }
    } else {
      std::cout << "Configuration file path should start with 'file:'" << std::endl;
    }

    return;
  }
  
  struct OptionsStruct 
  {
    std::string clock = "local";
    std::string datapathMode = "packet";
    std::string downstreamData = "Ctp";
    std::string gbtMode = "gbt";
    std::string gbtMux = "ttc";
    std::string links = "0";
    std::string configFile = "";
    bool linkLoopbackEnabled = false;
    bool configAll= false;
    bool forceConfig = false;
  }mOptions;

  private:
};

int main(int argc, char** argv)
{
  return ProgramConfig().execute(argc, argv);
}
