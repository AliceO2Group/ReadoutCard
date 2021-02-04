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

#include <fstream>
#include <iostream>
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/CardConfigurator.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/FirmwareChecker.h"
#include "ReadoutCard/Logger.h"
#include "RocPciDevice.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramConfig : public Program
{
 public:
  ProgramConfig(bool ilgEnabled) : Program(ilgEnabled)
  {
  }

  virtual Description getDescription()
  {
    return { "Config", "Configure the ReadoutCard(s)",
             "roc-config --config-uri ini:///home/flp/roc.cfg\n"
             "roc-config --id 42:00.0 --links 0-23 --clock local --datapathmode packet --loopback --gbtmux ttc #CRU\n"
             "roc-config --id #0 --crorc-id 0x42 --dyn-offset --tf-length 255 #CRORC\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()("allow-rejection",
                          po::bool_switch(&mOptions.allowRejection),
                          "Flag to allow HBF rejection");
    options.add_options()("clock",
                          po::value<std::string>(&mOptions.clock)->default_value("LOCAL"),
                          "Clock [LOCAL, TTC]");
    options.add_options()("crorc-id",
                          po::value<std::string>(&mOptions.crorcId)->default_value("0x0"),
                          "12-bit CRORC ID");
    options.add_options()("cru-id",
                          po::value<std::string>(&mOptions.cruId)->default_value("0x0"),
                          "12-bit CRU ID");
    options.add_options()("datapathmode",
                          po::value<std::string>(&mOptions.datapathMode)->default_value("PACKET"),
                          "DatapathMode [PACKET, STREAMING]");
    options.add_options()("downstreamdata",
                          po::value<std::string>(&mOptions.downstreamData)->default_value("CTP"),
                          "DownstreamData [CTP, PATTERN, MIDTRG]");
    options.add_options()("gbtmode",
                          po::value<std::string>(&mOptions.gbtMode)->default_value("GBT"),
                          "GBT MODE [GBT, WB]");
    options.add_options()("gbtmux",
                          po::value<std::string>(&mOptions.gbtMux)->default_value("TTC"),
                          "GBT MUX [TTC, DDG, SWT, TTCUP, UL]");
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
                          po::value<uint32_t>(&mOptions.onuAddress)->default_value(0),
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
    options.add_options()("trigger-window-size",
                          po::value<uint32_t>(&mOptions.triggerWindowSize),
                          "The size of the trigger window in GBT words");
    options.add_options()("tf-length",
                          po::value<uint32_t>(&mOptions.timeFrameLength),
                          "Sets the length of the Time Frame");
    options.add_options()("no-tf-detection",
                          po::bool_switch(&mOptions.timeFrameDetectionDisabled),
                          "Flag to enable the Time Frame Detection");
    options.add_options()("gen-cfg-file",
                          po::value<std::string>(&mOptions.genConfigFile),
                          "If set generates a configuration file from the command line options. [DOES NOT CONFIGURE]");
    options.add_options()("no-gbt",
                          po::bool_switch(&mOptions.noGbt),
                          "Flag to switch off GBT");
    options.add_options()("user-logic",
                          po::bool_switch(&mOptions.userLogicEnabled),
                          "Flag to toggle the User Logic link");
    options.add_options()("run-stats",
                          po::bool_switch(&mOptions.runStatsEnabled),
                          "Flag to toggle the Run Statistics link");
    options.add_options()("user-and-common-logic",
                          po::bool_switch(&mOptions.userAndCommonLogicEnabled),
                          "Flag to toggle the User and Common Logic");
    options.add_options()("system-id",
                          po::value<std::string>(&mOptions.systemId),
                          "Sets the System ID");
    options.add_options()("fee-id",
                          po::value<std::string>(&mOptions.feeId),
                          "Sets the FEE ID");
    Options::addOptionCardId(options);
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    Logger::setFacility("ReadoutCard/config");

    // Configure all cards found - Normally used during boot
    if (mOptions.configAll) {
      Logger::get() << "Running RoC Configuration for all cards" << LogInfoOps << endm;
      std::vector<CardDescriptor> cardsFound;
      if (mOptions.configUri == "") {
        Logger::get() << "A configuration URI is necessary with the startup-config flag set" << LogErrorOps << endm;
        return;
      }

      cardsFound = RocPciDevice::findSystemDevices();
      for (auto const& card : cardsFound) {
        Logger::get() << " __== " << card.pciAddress.toString() << " ==__ " << LogDebugTrace << endm;
        auto params = Parameters::makeParameters(card.pciAddress, 2);
        if (!mOptions.bypassFirmwareCheck) {
          try {
            FirmwareChecker().checkFirmwareCompatibility(params);
            CardConfigurator(card.pciAddress, mOptions.configUri, mOptions.forceConfig);
          } catch (const Exception& e) {
            Logger::get() << boost::diagnostic_information(e) << LogErrorOps << endm;
          }
        }
      }
      return;
    }

    // Configure specific card
    auto cardId = Options::getOptionCardId(map); //TODO: Parameters not planned for the CRORC should throw an exception when used
    if (!mOptions.bypassFirmwareCheck) {
      try {
        FirmwareChecker().checkFirmwareCompatibility(cardId);
      } catch (const Exception& e) {
        Logger::get() << boost::diagnostic_information(e) << LogErrorOps << endm;
        throw(e);
      }
    }

    if (mOptions.configUri == "") {
      auto params = Parameters::makeParameters(cardId, 2);
      params.setLinkMask(Parameters::linkMaskFromString(mOptions.links));
      params.setAllowRejection(mOptions.allowRejection);
      params.setClock(Clock::fromString(mOptions.clock));
      params.setCrorcId(strtoul(mOptions.crorcId.c_str(), NULL, 16)); //TODO: Clean up common / per type parameters
      params.setCruId(strtoul(mOptions.cruId.c_str(), NULL, 16));
      params.setDatapathMode(DatapathMode::fromString(mOptions.datapathMode));
      params.setDownstreamData(DownstreamData::fromString(mOptions.downstreamData));
      params.setGbtMode(GbtMode::fromString(mOptions.gbtMode));
      params.setGbtMux(GbtMux::fromString(mOptions.gbtMux));
      params.setLinkLoopbackEnabled(mOptions.linkLoopbackEnabled);
      params.setPonUpstreamEnabled(mOptions.ponUpstreamEnabled);
      params.setDynamicOffsetEnabled(mOptions.dynamicOffsetEnabled);
      params.setOnuAddress(mOptions.onuAddress);
      params.setTriggerWindowSize(mOptions.triggerWindowSize);
      params.setGbtEnabled(!mOptions.noGbt);
      params.setUserLogicEnabled(mOptions.userLogicEnabled);
      params.setRunStatsEnabled(mOptions.runStatsEnabled);
      params.setUserAndCommonLogicEnabled(mOptions.userAndCommonLogicEnabled);
      params.setTimeFrameLength(mOptions.timeFrameLength);
      params.setTimeFrameDetectionEnabled(!mOptions.timeFrameDetectionDisabled);
      params.setSystemId(strtoul(mOptions.systemId.c_str(), NULL, 16));
      params.setFeeId(strtoul(mOptions.feeId.c_str(), NULL, 16));

      // Generate a configuration file base on the parameters provided
      if (mOptions.genConfigFile != "") { //TODO: To be updated for the CRORC
        std::cout << "Generating a configuration file at: " << mOptions.genConfigFile << std::endl;
        std::ofstream cfgFile;
        cfgFile.open(mOptions.genConfigFile);

        cfgFile << "[cru]\n";
        cfgFile << "allowRejection=" << std::boolalpha << mOptions.allowRejection << "\n";
        cfgFile << "clock=" << mOptions.clock << "\n";
        cfgFile << "cruId=" << mOptions.cruId << "\n";
        cfgFile << "datapathMode=" << mOptions.datapathMode << "\n";
        cfgFile << "loopback=" << std::boolalpha << mOptions.linkLoopbackEnabled << "\n";
        cfgFile << "gbtMode=" << mOptions.gbtMode << "\n";
        cfgFile << "downstreamData=" << mOptions.downstreamData << "\n";
        cfgFile << "ponUpstream=" << std::boolalpha << mOptions.ponUpstreamEnabled << "\n";
        cfgFile << "onuAddress=" << mOptions.onuAddress << "\n";
        cfgFile << "dynamicOffset=" << std::boolalpha << mOptions.dynamicOffsetEnabled << "\n";
        cfgFile << "triggerWindowSize=" << mOptions.triggerWindowSize << "\n";
        cfgFile << "gbtEnabled=" << std::boolalpha << !mOptions.noGbt << "\n";
        cfgFile << "userLogicEnabled=" << std::boolalpha << mOptions.userLogicEnabled << "\n";
        cfgFile << "runStatsEnabled=" << std::boolalpha << mOptions.runStatsEnabled << "\n";
        cfgFile << "userAndCommonLogicEnabled=" << std::boolalpha << mOptions.userAndCommonLogicEnabled << "\n";
        cfgFile << "timeFrameLength=" << mOptions.timeFrameLength << "\n";
        cfgFile << "timeFrameDetectionEnabled=" << std::boolalpha << !mOptions.timeFrameDetectionDisabled << "\n";
        cfgFile << "systemId=" << mOptions.systemId << "\n";

        cfgFile << "[links]\n";
        cfgFile << "enabled=false\n";
        cfgFile << "gbtMux=TTC\n";
        cfgFile << "feeId=" << mOptions.systemId << "\n";

        for (const auto& link : params.getLinkMaskRequired()) {
          cfgFile << "[link" << link << "]\n";
          cfgFile << "enabled=true\n";
          cfgFile << "gbtMux=" << mOptions.gbtMux << "\n";
          cfgFile << "feeId=" << mOptions.systemId << "\n";
        }

        cfgFile.close();
        return;
      }

      Logger::get() << "Configuring with command line arguments" << LogDebugOps << endm;
      try {
        CardConfigurator(params, mOptions.forceConfig);
      } catch (const Exception& e) {
        Logger::get() << boost::diagnostic_information(e) << LogErrorOps << endm;
        throw(e);
      }
    } else {
      Logger::get() << "Configuring with config uri" << LogDebugOps << endm;
      try {
        CardConfigurator(cardId, mOptions.configUri, mOptions.forceConfig);
      } catch (std::runtime_error& e) {
        Logger::get() << "Error parsing the configuration..." << boost::diagnostic_information(e) << LogErrorOps << endm;
        throw(e);
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
    std::string genConfigFile = "";
    std::string links = "0";
    bool allowRejection = false;
    bool bypassFirmwareCheck = false;
    bool configAll = false;
    bool forceConfig = false;
    bool linkLoopbackEnabled = false;
    bool ponUpstreamEnabled = false;
    bool dynamicOffsetEnabled = false;
    uint32_t onuAddress = 0;
    std::string cruId = "0x0";
    std::string crorcId = "0x0";
    uint32_t triggerWindowSize = 1000;
    uint32_t timeFrameLength = 0x100;
    bool timeFrameDetectionDisabled = false;
    bool userLogicEnabled = false;
    bool runStatsEnabled = false;
    bool userAndCommonLogicEnabled = false;
    bool noGbt = false;
    std::string systemId = "0x0";
    std::string feeId = "0x0";
    /*std::string systemId = "0x1ff"; // TODO: Default values that can be used to check if params have been specified
    std::string feeId = "0x1f";*/
  } mOptions;

 private:
};

int main(int argc, char** argv)
{
  // true here enables InfoLogger output by default
  // see the Program constructor
  return ProgramConfig(true).execute(argc, argv);
}
