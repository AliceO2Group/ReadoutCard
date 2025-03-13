
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
/// \file ProgramConfig.cxx
/// \brief Tool that configures the CRU.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <fstream>
#include <iostream>
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Cru/CruBar.h"
#include "Crorc/CrorcBar.h"
#include "ReadoutCard/CardConfigurator.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/FirmwareChecker.h"
#include "ReadoutCard/Logger.h"
#include "RocPciDevice.h"
#include <InfoLogger/InfoLogger.hxx>
#include <boost/format.hpp>

using namespace AliceO2::InfoLogger;
using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
namespace po = boost::program_options;

std::string cmd; // program command invoked

namespace o2
{
namespace roc
{
extern bool testModeORC501; // testMode flag used for some FW dev, cf JIRA ORC-501
}
}

/// Get a status report of given card
std::string getStatusReport(Parameters::CardIdType cardId)
{

  auto card = RocPciDevice(cardId).getCardDescriptor();
  auto cardType = card.cardType;

  std::ostringstream table;
  std::string formatHeader;
  std::string formatRow;
  std::string header;
  std::string lineFat;
  std::string lineThin;
  const char* linkMask = "0-11";

  if (cardType == CardType::type::Crorc) {
    formatHeader = "  %-9s %-8s %-19s\n";
    formatRow = "  %-9s %-8s %-19.1f\n";
    header = (boost::format(formatHeader) % "Link ID" % "Status" % "Optical power(uW)").str();
    lineFat = std::string(header.length(), '=') + '\n';
    lineThin = std::string(header.length(), '-') + '\n';

    auto params = Parameters::makeParameters(cardId, 0); // status available on BAR0
    params.setLinkMask(Parameters::linkMaskFromString(linkMask));
    auto bar0 = ChannelFactory().getBar(params);
    auto crorcBar0 = std::dynamic_pointer_cast<CrorcBar>(bar0);

    Crorc::ReportInfo reportInfo = crorcBar0->report();
    std::string qsfpEnabled = reportInfo.qsfpEnabled ? "Enabled" : "Disabled";
    std::string offset = reportInfo.dynamicOffset ? "Dynamic" : "Fixed";
    std::string timeFrameDetectionEnabled = reportInfo.timeFrameDetectionEnabled ? "Enabled" : "Disabled";

    if (card.serialId.getSerial() == 0x7fffffff || card.serialId.getSerial() == 0x0) {
      table << "Bad serial reported, bad card state" << std::endl;
    } else {

      table << "-----------------------------" << std::endl;
      table << "QSFP " << qsfpEnabled << std::endl;
      table << offset << " offset" << std::endl;
      table << "-----------------------------" << std::endl;
      table << "Time Frame Detection " << timeFrameDetectionEnabled << std::endl;
      table << "Time Frame Length: " << reportInfo.timeFrameLength << std::endl;
      table << "-----------------------------" << std::endl;

      table << lineFat << header << lineThin;

      /* PARAMETERS PER LINK */
      for (const auto& el : reportInfo.linkMap) {
        auto link = el.second;
        int id = el.first;

        std::string linkStatus = link.status == Crorc::LinkStatus::Up ? "UP" : "DOWN";
        float opticalPower = link.opticalPower;

        auto format = boost::format(formatRow) % id % linkStatus % opticalPower;
        table << format;
      }
    }
    lineFat = std::string(header.length(), '=') + '\n';
    table << lineFat;

  } else if (cardType == CardType::type::Cru) {
    formatHeader = "  %-9s %-16s %-10s %-14s %-15s %-10s %-14s %-14s %-8s %-19s %-11s %-7s\n";
    formatRow = "  %-9s %-16s %-10s %-14s %-15s %-10s %-14.2f %-14.2f %-8s %-19.1f %-11s %-7s\n";
    header = (boost::format(formatHeader) % "Link ID" % "GBT Mode Tx/Rx" % "Loopback" % "GBT MUX" % "Datapath Mode" % "Datapath" % "RX freq(MHz)" % "TX freq(MHz)" % "Status" % "Optical power(uW)" % "System ID" % "FEE ID").str();
    lineFat = std::string(header.length(), '=') + '\n';
    lineThin = std::string(header.length(), '-') + '\n';

    auto params = Parameters::makeParameters(cardId, 2); // status available on BAR2
    params.setLinkMask(Parameters::linkMaskFromString(linkMask));
    auto bar2 = ChannelFactory().getBar(params);
    auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar2);

    Cru::ReportInfo reportInfo = cruBar2->report();

    std::string clock = (reportInfo.ttcClock == 0 ? "TTC" : "Local");
    std::string offset = (reportInfo.dynamicOffset ? "Dynamic" : "Fixed");
    std::string userLogic = (reportInfo.userLogicEnabled ? "Enabled" : "Disabled");
    std::string runStats = (reportInfo.runStatsEnabled ? "Enabled" : "Disabled");
    std::string userAndCommonLogic = (reportInfo.userAndCommonLogicEnabled ? "Enabled" : "Disabled");
    std::string dropBadRdh = (reportInfo.dropBadRdhEnabled ? "Enabled" : "Disabled");

    table << "-----------------------------" << std::endl;
    table << "CRU ID: " << reportInfo.cruId << std::endl;
    table << clock << " clock | ";
    table << offset << " offset" << std::endl;
    table << "Timeframe length: " << (int)reportInfo.timeFrameLength << std::endl;
    if (reportInfo.userLogicEnabled && reportInfo.userAndCommonLogicEnabled) {
      table << "User and Common Logic enabled" << std::endl;
    } else if (reportInfo.userLogicEnabled) {
      table << "User Logic enabled" << std::endl;
    }
    if (reportInfo.runStatsEnabled) {
      table << "Run statistics enabled" << std::endl;
    }
    if (reportInfo.dropBadRdhEnabled) {
      table << "Drop packets with bad RDH enabled" << std::endl;
    }

    Cru::OnuStatus onuStatus = cruBar2->reportOnuStatus(0);

    std::string onuUpstreamStatus = Cru::LinkStatusToString(onuStatus.stickyStatus.upstreamStatus);
    std::string onuDownstreamStatus = Cru::LinkStatusToString(onuStatus.stickyStatus.downstreamStatus);
    uint32_t onuStickyValue = onuStatus.stickyStatus.stickyValue;
    uint32_t onuStickyValuePrev = onuStatus.stickyStatus.stickyValuePrev;

    std::string ponQualityStatusStr;
    ponQualityStatusStr = onuStatus.ponQualityStatus ? "good" : "bad";

    table << "=============================" << std::endl;
    table << "ONU downstream status: " << onuDownstreamStatus << std::endl;
    table << "ONU upstream status: " << onuUpstreamStatus << std::endl;
    table << "ONU sticky value: 0x" << std::hex << onuStickyValue << std::endl;
    table << "ONU sticky value (was): 0x" << std::hex << onuStickyValuePrev << std::endl;
    table << "ONU address: " << onuStatus.onuAddress << std::endl;
    table << "-----------------------------" << std::endl;
    table << "ONU RX40 locked: " << onuStatus.rx40Locked << std::endl;
    table << "ONU phase good: " << onuStatus.phaseGood << std::endl;
    table << "ONU RX locked: " << onuStatus.rxLocked << std::endl;
    table << "ONU operational: " << onuStatus.operational << std::endl;
    table << "ONU MGT TX ready: " << onuStatus.mgtTxReady << std::endl;
    table << "ONU MGT RX ready: " << onuStatus.mgtRxReady << std::endl;
    table << "ONU MGT TX PLL locked: " << onuStatus.mgtTxPllLocked << std::endl;
    table << "ONU MGT RX PLL locked: " << onuStatus.mgtRxPllLocked << std::endl;
    table << "PON quality: 0x" << std::hex << onuStatus.ponQuality << std::endl;
    table << "PON quality status: " << ponQualityStatusStr << std::endl;
    table << "PON RX power (dBm): " << onuStatus.ponRxPower << std::endl;

    table << lineFat << header << lineThin;

    /* PARAMETERS PER LINK */
    for (const auto& el : reportInfo.linkMap) {
      auto link = el.second;
      int globalId = el.first; // Use the "new" link mapping
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
      std::string systemId = Utilities::toHexString(link.systemId);
      std::string feeId = Utilities::toHexString(link.feeId);
      auto format = boost::format(formatRow) % globalId % gbtTxRxMode % loopback % gbtMux % datapathMode % enabled % rxFreq % txFreq % linkStatus % opticalPower % systemId % feeId;
      table << format;
    }
    lineFat = std::string(header.length(), '=') + '\n';
    table << lineFat;
  }

  return table.str();
}

class ProgramConfig : public Program
{
 public:
  ProgramConfig(bool ilgEnabled) : Program(ilgEnabled)
  {
  }

  virtual Description getDescription()
  {
    return { "Config", "Configure the ReadoutCard(s)",
             "o2-roc-config --config-uri ini:///home/flp/roc.cfg\n"
             "o2-roc-config --id 42:00.0 --links 0-11 --clock local --datapathmode packet --loopback --gbtmux ttc #CRU\n"
             "o2-roc-config --id #0 --crorc-id 0x42 --dyn-offset --tf-length 255 #CRORC\n" };
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
                          po::value<uint32_t>(&mOptions.timeFrameLength)->default_value(0x100),
                          "Sets the length of the Time Frame");
    options.add_options()("no-tf-detection",
                          po::bool_switch(&mOptions.timeFrameDetectionDisabled),
                          "Flag to enable the Time Frame Detection");
    options.add_options()("gen-cfg-file",
                          po::value<std::string>(&mOptions.genConfigFile),
                          "If set generates a CRU configuration file from the command line options. [DOES NOT CONFIGURE]");
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
    options.add_options()("status-report",
                          po::value<std::string>(&mOptions.statusReport),
                          "Sets file where to output card status (similar to roc-status). Can be stdout, infologger, or a file name. The file name can be preceded with + for appending the file. Name can contain special escape sequences %t (timestamp) %T (date/time) or %i (card ID). Infologger reports are set with error code 4805.");
    options.add_options()("drop-bad-rdh",
                          po::bool_switch(&mOptions.dropBadRdhEnabled),
                          "Flag to enable dropping of packets with bad RDH");
    options.add_options()("test-mode-ORC501",
                          po::bool_switch(&o2::roc::testModeORC501),
                          "Flag to enable test mode as described in JIRA ORC-501");
    Options::addOptionCardId(options);
  }

  static std::string cardIdToString(const Parameters::CardIdType& cardId)
  {
    if (auto* id = boost::get<o2::roc::PciAddress>(&cardId)) {
      return id->toString();
    } else if (auto* id = boost::get<o2::roc::PciSequenceNumber>(&cardId)) {
      return id->toString();
    } else if (auto* id = boost::get<o2::roc::SerialId>(&cardId)) {
      return id->toString();
    }
    return "";
  }

  virtual void reportStatus(Parameters::CardIdType cardId)
  {
    if (mOptions.statusReport != "") {

      // create report
      std::string report;

      // time now
      std::time_t t = std::time(nullptr);
      std::tm tm = *std::localtime(&t);
      std::stringstream buffer;
      buffer << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

      report += "roc-config execution report\n";
      report += "Card:           " + cardIdToString(cardId) + "\n";
      report += "Time completed: " + buffer.str() + "\n";
      report += "Command:        " + cmd + "\n";

      // do as in roc-status
      report += "Status: \n" + getStatusReport(cardId);

      // parse filename
      std::string fileName;
      int parseError = 0;
      const char* fileMode = "w";
      for (std::string::iterator it = mOptions.statusReport.begin(); it != mOptions.statusReport.end(); ++it) {
        if (*it == '%') {
          // escape characters
          ++it;
          if (it != fileName.end()) {
            if (*it == 't') {
              fileName += std::to_string(std::time(nullptr));
            } else if (*it == 'T') {
              std::stringstream buffer;
              buffer << std::put_time(&tm, "%Y_%m_%d__%H_%M_%S");
              fileName += buffer.str();
            } else if (*it == 'i') {
              fileName += cardIdToString(cardId);
            }
          } else {
            parseError++;
          }
        } else if ((it == mOptions.statusReport.begin()) && (*it == '+')) {
          fileMode = "a";
        } else {
          // normal char - copy it
          fileName += *it;
        }
        if (parseError) {
          break;
        }
      }

      // write report
      if (fileName == "stdout") {
        printf("\n%s\n", report.c_str());
      } else if (fileName == "infologger") {
        InfoLogger theLog;
        InfoLoggerContext theLogContext;
        theLogContext.setField(InfoLoggerContext::FieldName::Facility, ilFacility);
        theLog.setContext(theLogContext);
        std::string line;
        std::stringstream ss;
        ss << report;
        while (std::getline(ss, line)) {
          theLog << LogInfoDevel_(4805) << line << InfoLogger::endm;
        }
      } else {
        FILE* fp = fopen(fileName.c_str(), fileMode);
        if (fp == nullptr) {
          BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Failed to open report file " + fileName + " : " + strerror(errno)));
        } else {
          fprintf(fp, "%s\n", report.c_str());
          fclose(fp);
        }
      }
    }
    return;
  }

  const char* ilFacility = "ReadoutCard/config";

  virtual void run(const boost::program_options::variables_map& map)
  {

    Logger::setFacility(ilFacility);

    // Configure all cards found - Normally used during boot
    if (mOptions.configAll) {
      Logger::get() << "Running RoC Configuration for all cards" << LogInfoDevel_(4600) << endm;
      std::vector<CardDescriptor> cardsFound;
      if (mOptions.configUri == "") {
        Logger::get() << "A configuration URI is necessary with the startup-config flag set" << LogErrorDevel_(4600) << endm;
        return;
      }

      cardsFound = RocPciDevice::findSystemDevices();
      for (auto const& card : cardsFound) {
        Logger::get() << " __== " << card.pciAddress.toString() << " ==__ " << LogDebugTrace_(4600) << endm;
        auto params = Parameters::makeParameters(card.pciAddress, 2);
        if (!mOptions.bypassFirmwareCheck) {
          try {
            FirmwareChecker().checkFirmwareCompatibility(params);
            CardConfigurator(card.pciAddress, mOptions.configUri, mOptions.forceConfig);
            reportStatus(card.pciAddress);
          } catch (const std::runtime_error& e) {
            Logger::get() << e.what() << LogErrorDevel_(4600) << endm;
          } catch (const Exception& e) {
            Logger::get() << boost::diagnostic_information(e) << LogErrorDevel_(4600) << endm;
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
        Logger::get() << boost::diagnostic_information(e) << LogErrorDevel_(4600) << endm;
        throw;
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
      params.setDropBadRdhEnabled(mOptions.dropBadRdhEnabled);

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
        //cfgFile << "timeFrameDetectionEnabled=" << std::boolalpha << !mOptions.timeFrameDetectionDisabled << "\n";
        cfgFile << "systemId=" << mOptions.systemId << "\n";
        cfgFile << "timeFrameLength=" << mOptions.timeFrameLength << "\n";
        cfgFile << "dropBadRdhEnabled=" << std::boolalpha << mOptions.dropBadRdhEnabled << "\n";

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

      Logger::get() << "Configuring card " << cardId << " with command line arguments" << LogDebugDevel_(4600) << endm;
      if (mOptions.forceConfig) {
        Logger::get() << "`--force` enabled" << LogDebugDevel_(4600) << endm;
      }

      try {
        CardConfigurator(params, mOptions.forceConfig);
        reportStatus(cardId);
      } catch (const std::runtime_error& e) {
        Logger::get() << e.what() << LogErrorDevel_(4600) << endm;
        throw;
      } catch (const Exception& e) {
        Logger::get() << e.what() << LogErrorDevel_(4600) << endm;
        throw;
      }
    } else {
      Logger::get() << "Configuring card " << cardId << " with config uri: " << mOptions.configUri << LogDebugDevel_(4600) << endm;
      if (mOptions.forceConfig) {
        Logger::get() << "`--force` enabled" << LogDebugDevel_(4600) << endm;
      }

      try {
        CardConfigurator(cardId, mOptions.configUri, mOptions.forceConfig);
        reportStatus(cardId);
      } catch (const std::runtime_error& e) {
        Logger::get() << e.what() << LogErrorDevel_(4600) << endm;
        throw;
      } catch (const Exception& e) {
        Logger::get() << e.what() << LogErrorDevel_(4600) << endm;
        throw;
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
    std::string statusReport = ""; // when set, output roc status to file
    /*std::string systemId = "0x1ff"; // TODO: Default values that can be used to check if params have been specified
    std::string feeId = "0x1f";*/
    bool dropBadRdhEnabled = false;
  } mOptions;

 private:
};

int main(int argc, char** argv)
{
  for (int i = 0; i < argc; i++) {
    cmd += argv[i] + std::string(" ");
  }
  // true here enables InfoLogger output by default
  // see the Program constructor
  return ProgramConfig(true).execute(argc, argv);
}
