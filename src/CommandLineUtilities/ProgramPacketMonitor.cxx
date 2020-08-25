// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramPacketMonitor.cxx
/// \brief Tool that returns monitoring information about RoC packets.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Cru/CruBar.h"
#include "Crorc/CrorcBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Monitoring/MonitoringFactory.h"
using namespace o2::monitoring;

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace pt = boost::property_tree;
namespace po = boost::program_options;

class ProgramPacketMonitor : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Packet Monitor", "Return RoC packet monitoring information",
             "roc-pkt-monitor --id 42:00.0\n"
             "roc-pkt-monitor --id 42:00.0 --json\n"
             "roc-pkt-monitor --id 42:00.0 --monitoring\n" };
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
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    auto cardId = Options::getOptionCardId(map);
    Parameters params;
    std::shared_ptr<BarInterface> bar;
    auto card = RocPciDevice(cardId).getCardDescriptor();
    auto cardType = card.cardType;

    // Monitoring instance to send metrics
    std::unique_ptr<Monitoring> monitoring;
    if (mOptions.monitoring) {
      monitoring = MonitoringFactory::Get(getMonitoringUri());
    }

    if (cardType == CardType::type::Crorc) {

      /* HEADER */
      std::ostringstream table;
      auto formatHeader = "  %-9s %-10s %-18s\n";
      auto formatRow = "  %-9s %-10s %-18s\n";
      auto header = (boost::format(formatHeader) % "Channel" % "ACQ Rate" % "Packets Received").str();
      auto lineFat = std::string(header.length(), '=') + '\n';
      auto lineThin = std::string(header.length(), '-') + '\n';

      if (!mOptions.jsonOut && !mOptions.monitoring) {
        table << lineFat << header << lineThin;
      }

      // initialize ptrees
      pt::ptree root;
      pt::ptree links;

      /* TABLE */
      for (int channel = 0; channel < 6; channel++) {
        params = Parameters::makeParameters(cardId, channel);
        bar = ChannelFactory().getBar(params);
        auto crorcBar = std::dynamic_pointer_cast<CrorcBar>(bar);
        Crorc::PacketMonitoringInfo packetMonitoringInfo = crorcBar->monitorPackets();
        uint32_t acquisitionRate = packetMonitoringInfo.acquisitionRate;
        uint32_t packetsReceived = packetMonitoringInfo.packetsReceived;

        if (mOptions.monitoring) {
          monitoring->send(Metric{ "link" }
                             .addValue(card.pciAddress.toString(), "pciAddress")
                             .addValue((int)acquisitionRate, "acquisitionRate")
                             .addValue((int)packetsReceived, "packetsReceived")
                             .addTag(tags::Key::SerialId, card.serialId.getSerial())
                             .addTag(tags::Key::CRORC, card.sequenceId)
                             .addTag(tags::Key::ID, channel)
                             .addTag(tags::Key::Type, tags::Value::CRORC));
        } else if (mOptions.jsonOut) {
          pt::ptree linkNode;

          linkNode.put("linkId", std::to_string(channel));
          linkNode.put("acquisitionRate", std::to_string(acquisitionRate));
          linkNode.put("packetsReceived", std::to_string(packetsReceived));

          links.add_child(std::to_string(channel), linkNode);
        } else {
          auto format = boost::format(formatRow) % channel % acquisitionRate % packetsReceived;
          table << format;
        }
      }

      /* PRINT */
      if (mOptions.jsonOut) {
        root.add_child("links", links);
        pt::write_json(std::cout, root);
      } else if (!mOptions.monitoring) {
        table << lineFat;
        std::cout << table.str();
      }

      return;
    } else if (cardType == CardType::type::Cru) {

      params = Parameters::makeParameters(cardId, 2); //status available on BAR2
      bar = ChannelFactory().getBar(params);
      auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar);

      Cru::PacketMonitoringInfo packetMonitoringInfo = cruBar2->monitorPackets();

      /* HEADER */
      std::ostringstream table;
      auto formatHeader = "  %-9s %-14s %-14s %-12s\n";
      auto formatRow = "  %-9s %-14s %-14s %-12s\n";
      auto header = (boost::format(formatHeader) % "Link ID" % "Accepted" % "Rejected" % "Forced").str();
      auto lineFat = std::string(header.length(), '=') + '\n';
      auto lineThin = std::string(header.length(), '-') + '\n';

      if (!mOptions.jsonOut && !mOptions.monitoring) {
        table << lineFat << header << lineThin;
      }

      // initialize ptrees
      pt::ptree root;
      pt::ptree gbtLinks;

      /* TABLE */
      for (const auto& el : packetMonitoringInfo.linkPacketInfoMap) {
        int globalId = el.first;
        auto linkMonitoringInfoMap = el.second;
        uint32_t accepted = linkMonitoringInfoMap.accepted;
        uint32_t rejected = linkMonitoringInfoMap.rejected;
        uint32_t forced = linkMonitoringInfoMap.forced;

        if (mOptions.monitoring) {
          monitoring->send(Metric{ "link" }
                             .addValue(card.pciAddress.toString(), "pciAddress")
                             .addValue((int)accepted, "accepted")
                             .addValue((int)rejected, "rejected")
                             .addValue((int)forced, "forced")
                             .addTag(tags::Key::SerialId, card.serialId.getSerial())
                             .addTag(tags::Key::Endpoint, card.serialId.getEndpoint())
                             .addTag(tags::Key::CRU, card.sequenceId)
                             .addTag(tags::Key::ID, globalId)
                             .addTag(tags::Key::Type, tags::Value::CRU));

        } else if (mOptions.jsonOut) {
          pt::ptree linkNode;

          // add kv pairs for this link
          linkNode.put("linkId", std::to_string(globalId));
          linkNode.put("accepted", std::to_string(accepted));
          linkNode.put("rejected", std::to_string(rejected));
          linkNode.put("forced", std::to_string(forced));

          gbtLinks.add_child(std::to_string(globalId), linkNode);
        } else {
          auto format = boost::format(formatRow) % globalId % accepted % rejected % forced;
          table << format;
        }
      }

      /* PRINT */
      if (mOptions.jsonOut) {
        // add links nodes to the tree
        root.add_child("gbtLinks", gbtLinks);
      } else if (!mOptions.monitoring) {
        std::cout << table.str();
      }

      /* HEADER */
      std::ostringstream otherTable;
      formatHeader = "  %-9s %-16s %-25s\n";
      formatRow = "  %-9s %-16s %-25s\n";
      header = (boost::format(formatHeader) % "Wrapper" % "Dropped" % "Total Packets per second").str();
      lineFat = std::string(header.length(), '=') + '\n';
      lineThin = std::string(header.length(), '-') + '\n';

      if (!mOptions.jsonOut && !mOptions.monitoring) {
        otherTable << lineFat << header << lineThin;
      }

      pt::ptree ulLinks;

      /* TABLE */
      for (const auto& el : packetMonitoringInfo.wrapperPacketInfoMap) {
        int wrapper = el.first;
        auto wrapperMonitoringInfoMap = el.second;
        uint32_t dropped = wrapperMonitoringInfoMap.dropped;
        uint32_t totalPacketsPerSec = wrapperMonitoringInfoMap.totalPacketsPerSec;

        if (mOptions.monitoring) {
          monitoring->send(Metric{ "wrapper" }
                             .addValue(card.pciAddress.toString(), "pciAddress")
                             .addValue((int)dropped, "dropped")
                             .addValue((int)totalPacketsPerSec, "totalPacketsPerSec")
                             .addTag(tags::Key::SerialId, card.serialId.getSerial())
                             .addTag(tags::Key::Endpoint, card.serialId.getEndpoint())
                             .addTag(tags::Key::CRU, card.sequenceId)
                             .addTag(tags::Key::ID, wrapper)
                             .addTag(tags::Key::Type, tags::Value::CRU));
        } else if (mOptions.jsonOut) {
          pt::ptree wrapperNode;

          // add kv pairs for this wrapper
          wrapperNode.put("wrapperId", std::to_string(wrapper));
          wrapperNode.put("dropped", std::to_string(dropped));
          wrapperNode.put("totalPacketsPerSec", std::to_string(totalPacketsPerSec));

          // add the wrapper node to the tree
          root.add_child("wrapper", wrapperNode);
        }
        if (!mOptions.jsonOut && !mOptions.monitoring) {
          auto format = boost::format(formatRow) % wrapper % dropped % totalPacketsPerSec;
          otherTable << format;
        }
      }

      /* BREAK + PRINT */
      if (mOptions.jsonOut) {
        pt::write_json(std::cout, root);
      } else if (!mOptions.monitoring) {
        auto lineFat = std::string(header.length(), '=') + '\n';
        otherTable << lineFat;
        std::cout << otherTable.str();
      }
    } else if (cardType != CardType::type::Cru) {
      std::cout << "Invalid card type" << std::endl;
      return;
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
  return ProgramPacketMonitor().execute(argc, argv);
}
