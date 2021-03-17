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
#include "Crorc/Common.h"
#include "Crorc/CrorcBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Utilities/Util.h"
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Monitoring/MonitoringFactory.h"
using namespace o2::monitoring;

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
namespace pt = boost::property_tree;
namespace po = boost::program_options;

class ProgramStatus : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Status", "Return current RoC configuration status",
             "roc-status --id 42:00.0\n"
             "roc-status --id 42:00.0 --json\n"
             "roc-status --id 42:00.0 --monitoring\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("json-out",
                          po::bool_switch(&mOptions.jsonOut),
                          "Toggle json-formatted output");
    options.add_options()("monitoring",
                          po::bool_switch(&mOptions.monitoring),
                          "Toggle monitoring metrics sending");
    options.add_options()("onu-status",
                          po::bool_switch(&mOptions.onu),
                          "Toggle ONU status output");
    options.add_options()("fec-status",
                          po::bool_switch(&mOptions.fec),
                          "Toggle FEC status output");
    options.add_options()("links",
                          po::value<std::string>(&mOptions.links)->default_value("0-11"),
                          "Links to show (all by default)");
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

    // Monitoring instance to send metrics
    std::unique_ptr<Monitoring> monitoring;
    if (mOptions.monitoring) {
      monitoring = MonitoringFactory::Get(getMonitoringUri());
    }

    if (cardType == CardType::type::Crorc) {
      formatHeader = "  %-9s %-8s %-19s\n";
      formatRow = "  %-9s %-8s %-19.1f\n";
      header = (boost::format(formatHeader) % "Link ID" % "Status" % "Optical power(uW)").str();
      lineFat = std::string(header.length(), '=') + '\n';
      lineThin = std::string(header.length(), '-') + '\n';

      if (!mOptions.jsonOut) {
        table << lineFat << header << lineThin;
      }

      auto params = Parameters::makeParameters(cardId, 0); //status available on BAR0
      params.setLinkMask(Parameters::linkMaskFromString(mOptions.links));
      auto bar0 = ChannelFactory().getBar(params);
      auto crorcBar0 = std::dynamic_pointer_cast<CrorcBar>(bar0);

      Crorc::ReportInfo reportInfo = crorcBar0->report();
      std::string qsfpEnabled = reportInfo.qsfpEnabled ? "Enabled" : "Disabled";
      std::string offset = reportInfo.dynamicOffset ? "Dynamic" : "Fixed";
      std::string timeFrameDetectionEnabled = reportInfo.timeFrameDetectionEnabled ? "Enabled" : "Disabled";

      if (card.serialId.getSerial() == 0x7fffffff || card.serialId.getSerial() == 0x0) {
        std::cout << "Bad serial reported, bad card state, exiting" << std::endl;
        return;
      }

      /* GENERAL PARAMETERS */
      if (mOptions.monitoring) {
        monitoring->send(Metric{ "CRORC" }
                           .addValue(card.pciAddress.toString(), "pciAddress")
                           .addValue(reportInfo.qsfpEnabled, "qsfp")
                           .addValue(reportInfo.dynamicOffset, "dynamicOffset")
                           .addValue(reportInfo.timeFrameDetectionEnabled, "timeFrameDetection")
                           .addValue(reportInfo.timeFrameLength, "timeFrameLength")
                           .addTag(tags::Key::SerialId, card.serialId.getSerial())
                           .addTag(tags::Key::ID, card.sequenceId)
                           .addTag(tags::Key::Type, tags::Value::CRORC));
      } else if (mOptions.jsonOut) {
        root.put("pciAddress", card.pciAddress.toString());
        root.put("serial", card.serialId.getSerial());
        root.put("qsfp", qsfpEnabled);
        root.put("offset", offset);
        root.put("timeFrameDetection", timeFrameDetectionEnabled);
        root.put("timeFrameLength", reportInfo.timeFrameLength);
      } else {
        std::cout << "-----------------------------" << std::endl;
        std::cout << "QSFP " << qsfpEnabled << std::endl;
        std::cout << offset << " offset" << std::endl;
        std::cout << "-----------------------------" << std::endl;
        std::cout << "Time Frame Detection " << timeFrameDetectionEnabled << std::endl;
        std::cout << "Time Frame Length: " << reportInfo.timeFrameLength << std::endl;
        std::cout << "-----------------------------" << std::endl;
      }

      /* PARAMETERS PER LINK */
      for (const auto& el : reportInfo.linkMap) {
        auto link = el.second;
        int id = el.first;

        std::string linkStatus = link.status == Crorc::LinkStatus::Up ? "UP" : "DOWN";
        float opticalPower = link.opticalPower;

        if (mOptions.monitoring) {
          monitoring->send(Metric{ "link" }
                             .addValue(card.pciAddress.toString(), "pciAddress")
                             .addValue(link.status, "status")
                             .addValue(opticalPower, "opticalPower")
                             .addTag(tags::Key::SerialId, card.serialId.getSerial())
                             .addTag(tags::Key::CRORC, card.sequenceId)
                             .addTag(tags::Key::ID, id)
                             .addTag(tags::Key::Type, tags::Value::CRORC));
        } else if (mOptions.jsonOut) {
          pt::ptree linkNode;

          // add kv pairs for this card
          linkNode.put("status", linkStatus);
          linkNode.put("opticalPower", Utilities::toPreciseString(opticalPower));

          // add the link node to the tree
          root.add_child(std::to_string(id), linkNode);
        } else {
          auto format = boost::format(formatRow) % id % linkStatus % opticalPower;
          table << format;
        }
      }
    } else if (cardType == CardType::type::Cru) {
      formatHeader = "  %-9s %-16s %-10s %-14s %-15s %-10s %-14s %-14s %-8s %-19s %-11s %-7s\n";
      formatRow = "  %-9s %-16s %-10s %-14s %-15s %-10s %-14.2f %-14.2f %-8s %-19.1f %-11s %-7s\n";
      header = (boost::format(formatHeader) % "Link ID" % "GBT Mode Tx/Rx" % "Loopback" % "GBT MUX" % "Datapath Mode" % "Datapath" % "RX freq(MHz)" % "TX freq(MHz)" % "Status" % "Optical power(uW)" % "System ID" % "FEE ID").str();
      lineFat = std::string(header.length(), '=') + '\n';
      lineThin = std::string(header.length(), '-') + '\n';

      auto params = Parameters::makeParameters(cardId, 2); //status available on BAR2
      params.setLinkMask(Parameters::linkMaskFromString(mOptions.links));
      auto bar2 = ChannelFactory().getBar(params);
      auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar2);

      if (!mOptions.jsonOut) {
        table << lineFat << header << lineThin;
      }

      Cru::ReportInfo reportInfo = cruBar2->report();

      std::string clock = (reportInfo.ttcClock == 0 ? "TTC" : "Local");
      std::string offset = (reportInfo.dynamicOffset ? "Dynamic" : "Fixed");
      std::string userLogic = (reportInfo.userLogicEnabled ? "Enabled" : "Disabled");
      std::string runStats = (reportInfo.runStatsEnabled ? "Enabled" : "Disabled");
      std::string userAndCommonLogic = (reportInfo.userAndCommonLogicEnabled ? "Enabled" : "Disabled");

      /* GENERAL PARAMETERS */
      if (mOptions.monitoring) {
        monitoring->send(Metric{ "CRU" }
                           .addValue(card.pciAddress.toString(), "pciAddress")
                           .addValue(reportInfo.cruId, "cruId")
                           .addValue(clock, "clock")
                           .addValue(reportInfo.dynamicOffset, "dynamicOffset")
                           .addValue(reportInfo.userLogicEnabled, "userLogic")
                           .addValue(reportInfo.runStatsEnabled, "runStats")
                           .addValue(reportInfo.userAndCommonLogicEnabled, "userAndCommonLogic")
                           .addTag(tags::Key::SerialId, card.serialId.getSerial())
                           .addTag(tags::Key::Endpoint, card.serialId.getEndpoint())
                           .addTag(tags::Key::ID, card.sequenceId)
                           .addTag(tags::Key::Type, tags::Value::CRU));
      } else if (mOptions.jsonOut) {
        root.put("pciAddress", card.pciAddress.toString());
        root.put("serial", card.serialId.getSerial());
        root.put("endpoint", card.serialId.getEndpoint());
        root.put("cruId", reportInfo.cruId);
        root.put("clock", clock);
        root.put("offset", offset);
        root.put("userLogic", userLogic);
        root.put("runStats", runStats);
        root.put("userAndCommonLogic", userAndCommonLogic);
      } else {
        std::cout << "-----------------------------" << std::endl;
        std::cout << "CRU ID: " << reportInfo.cruId << std::endl;
        std::cout << clock << " clock | ";
        std::cout << offset << " offset" << std::endl;
        if (reportInfo.userLogicEnabled && reportInfo.userAndCommonLogicEnabled) {
          std::cout << "User and Common Logic enabled" << std::endl;
        } else if (reportInfo.userLogicEnabled) {
          std::cout << "User Logic enabled" << std::endl;
        }
        if (reportInfo.runStatsEnabled) {
          std::cout << "Run statistics enabled" << std::endl;
        }
      }

      /* ONU PARAMETERS */
      if (mOptions.onu) {
        Cru::OnuStatus onuStatus = cruBar2->reportOnuStatus();

        std::string onuStickyStatus;
        int onuStickyStatusInt = 0;

        if (onuStatus.stickyBit == Cru::LinkStatus::Up) {
          onuStickyStatus = "UP";
          onuStickyStatusInt = 1;
        } else if (onuStatus.stickyBit == Cru::LinkStatus::UpWasDown) {
          onuStickyStatus = "UP (was DOWN)";
          // force status = 1 (vs = 2) when UP(was DOWN) for monitoring
          onuStickyStatusInt = 1;
        } else if (onuStatus.stickyBit == Cru::LinkStatus::Down) {
          onuStickyStatus = "DOWN";
          onuStickyStatusInt = 0;
        }

        std::string ponQualityStatusStr;
        ponQualityStatusStr = onuStatus.ponQualityStatus ? "good" : "bad";

        if (mOptions.monitoring) {
          monitoring->send(Metric{ "onu" }
                             .addValue(onuStickyStatusInt, "onuStickyStatus")
                             .addValue(std::to_string(onuStatus.onuAddress), "onuAddress")
                             .addValue(onuStatus.rx40Locked, "rx40Locked")
                             .addValue(onuStatus.phaseGood, "phaseGood")
                             .addValue(onuStatus.rxLocked, "rxLocked")
                             .addValue(onuStatus.operational, "operational")
                             .addValue(onuStatus.mgtTxReady, "mgtTxReady")
                             .addValue(onuStatus.mgtRxReady, "mgtRxReady")
                             .addValue(onuStatus.mgtTxPllLocked, "mgtTxPllLocked")
                             .addValue(onuStatus.mgtRxPllLocked, "mgtRxPllLocked")
                             .addValue(onuStatus.ponQualityStatus, "ponQualityStatus")
                             .addValue(onuStatus.ponRxPower, "ponRxPower")
                             .addTag(tags::Key::SerialId, card.serialId.getSerial())
                             .addTag(tags::Key::Endpoint, card.serialId.getEndpoint())
                             .addTag(tags::Key::ID, card.sequenceId)
                             .addTag(tags::Key::Type, tags::Value::CRU));
        } else if (mOptions.jsonOut) {
          root.put("ONU status", onuStickyStatus);
          root.put("ONU address", onuStatus.onuAddress);
          root.put("ONU RX40 locked", onuStatus.rx40Locked);
          root.put("ONU phase good", onuStatus.phaseGood);
          root.put("ONU RX locked", onuStatus.rxLocked);
          root.put("ONU operational", onuStatus.operational);
          root.put("ONU MGT TX ready", onuStatus.mgtTxReady);
          root.put("ONU MGT RX ready", onuStatus.mgtRxReady);
          root.put("ONU MGT TX PLL locked", onuStatus.mgtTxPllLocked);
          root.put("ONU MGT RX PLL locked", onuStatus.mgtRxPllLocked);
          root.put("PON quality", Utilities::toHexString(onuStatus.ponQuality));
          root.put("PON quality Status", ponQualityStatusStr);
          root.put("PON RX power (dBm)", onuStatus.ponRxPower);
        } else {
          std::cout << "=============================" << std::endl;
          std::cout << "ONU status: \t\t" << onuStickyStatus << std::endl;
          std::cout << "ONU address: \t\t0x" << std::hex << onuStatus.onuAddress << std::endl;
          std::cout << "-----------------------------" << std::endl;
          std::cout << "ONU RX40 locked: \t" << std::boolalpha << onuStatus.rx40Locked << std::endl;
          std::cout << "ONU phase good: \t" << std::boolalpha << onuStatus.phaseGood << std::endl;
          std::cout << "ONU RX locked: \t\t" << std::boolalpha << onuStatus.rxLocked << std::endl;
          std::cout << "ONU operational: \t" << std::boolalpha << onuStatus.operational << std::endl;
          std::cout << "ONU MGT TX ready: \t" << std::boolalpha << onuStatus.mgtTxReady << std::endl;
          std::cout << "ONU MGT RX ready: \t" << std::boolalpha << onuStatus.mgtRxReady << std::endl;
          std::cout << "ONU MGT TX PLL locked: \t" << std::boolalpha << onuStatus.mgtTxPllLocked << std::endl;
          std::cout << "ONU MGT RX PLL locked: \t" << std::boolalpha << onuStatus.mgtRxPllLocked << std::endl;
          std::cout << "PON quality: \t\t0x" << std::hex << onuStatus.ponQuality << std::endl;
          std::cout << "PON quality status: \t" << ponQualityStatusStr << std::endl;
          std::cout << "PON RX power (dBm): \t" << onuStatus.ponRxPower << std::endl;
        }
      }

      /* FEC PARAMETERS */
      if (mOptions.fec) {
        Cru::FecStatus fecStatus = cruBar2->reportFecStatus();

        if (mOptions.monitoring) {
          monitoring->send(Metric{ "fec" }
                             .addValue(fecStatus.clearFecCrcError, "clearFecCrcErrors")
                             .addValue(fecStatus.latchFecCrcError, "latchFecCrcErrors")
                             .addValue(fecStatus.slowControlFramingLocked, "slowControlFramingLocked")
                             .addValue(fecStatus.fecSingleErrorCount, "fecSingleErrorCount")
                             .addValue(fecStatus.fecDoubleErrorCount, "fecDoubleErrorCount")
                             .addValue(fecStatus.crcErrorCount, "crcErrorCount")
                             .addTag(tags::Key::SerialId, card.serialId.getSerial())
                             .addTag(tags::Key::Endpoint, card.serialId.getEndpoint())
                             .addTag(tags::Key::ID, card.sequenceId)
                             .addTag(tags::Key::Type, tags::Value::CRU));
        } else if (mOptions.jsonOut) {
          root.put("clearFecCrcErrors", fecStatus.clearFecCrcError);
          root.put("latchFecCrcErrors", fecStatus.latchFecCrcError);
          root.put("slowControlFramingLocked", fecStatus.slowControlFramingLocked);
          root.put("fecSingleErrorCount", fecStatus.fecSingleErrorCount);
          root.put("fecDoubleErrorCount", fecStatus.fecDoubleErrorCount);
          root.put("crcErrorCount", fecStatus.crcErrorCount);
        } else {
          std::cout << "=====================================" << std::endl;
          std::cout << "Clear FEC & CRC errors: \t" << std::boolalpha << fecStatus.clearFecCrcError << std::endl;
          std::cout << "Latch FEC & CRC errors: \t" << std::boolalpha << fecStatus.latchFecCrcError << std::endl;
          std::cout << "Slow Control Framing locked: \t" << std::boolalpha << fecStatus.slowControlFramingLocked << std::endl;
          std::cout << "FEC single error count: \t"
                    << "0x" << std::hex << fecStatus.fecSingleErrorCount << std::endl;
          std::cout << "FEC double error count: \t"
                    << "0x" << std::hex << fecStatus.fecDoubleErrorCount << std::endl;
          std::cout << "CRC error count: \t\t"
                    << "0x" << std::hex << fecStatus.crcErrorCount << std::endl;
        }
      }

      /* PARAMETERS PER LINK */
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
        std::string systemId = Utilities::toHexString(link.systemId);
        std::string feeId = Utilities::toHexString(link.feeId);

        if (mOptions.monitoring) {
          monitoring->send(Metric{ "link" }
                             .addValue(card.pciAddress.toString(), "pciAddress")
                             .addValue(gbtTxRxMode, "gbtMode")
                             .addValue(link.loopback, "loopback")
                             .addValue(gbtMux, "gbtMux")
                             .addValue(datapathMode, "datapathMode")
                             .addValue(link.enabled, "datapath")
                             .addValue(rxFreq, "rxFreq")
                             .addValue(txFreq, "txFreq")
                             .addValue(link.stickyBit, "status")
                             .addValue(opticalPower, "opticalPower")
                             .addValue(systemId, "systemId")
                             .addValue(feeId, "feeId")
                             .addTag(tags::Key::SerialId, card.serialId.getSerial())
                             .addTag(tags::Key::Endpoint, card.serialId.getEndpoint())
                             .addTag(tags::Key::CRU, card.sequenceId)
                             .addTag(tags::Key::ID, globalId)
                             .addTag(tags::Key::Type, tags::Value::CRU));
        } else if (mOptions.jsonOut) {
          pt::ptree linkNode;

          // add kv pairs for this card
          linkNode.put("gbtMode", gbtTxRxMode);
          linkNode.put("loopback", loopback);
          linkNode.put("gbtMux", gbtMux);
          linkNode.put("datapathMode", datapathMode);
          linkNode.put("datapath", enabled);
          linkNode.put("rxFreq", Utilities::toPreciseString(rxFreq));
          linkNode.put("txFreq", Utilities::toPreciseString(txFreq));
          linkNode.put("status", linkStatus);
          linkNode.put("opticalPower", Utilities::toPreciseString(opticalPower));
          linkNode.put("systemId", systemId);
          linkNode.put("feeId", feeId);

          // add the link node to the tree
          root.add_child(std::to_string(globalId), linkNode);
        } else {
          auto format = boost::format(formatRow) % globalId % gbtTxRxMode % loopback % gbtMux % datapathMode % enabled % rxFreq % txFreq % linkStatus % opticalPower % systemId % feeId;
          table << format;
        }
      }
    } else {
      std::cout << "Invalid card type" << std::endl;
      return;
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
    std::string links = "0-11";
    bool jsonOut = false;
    bool monitoring = false;
    bool onu = false;
    bool fec = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramStatus().execute(argc, argv);
}
