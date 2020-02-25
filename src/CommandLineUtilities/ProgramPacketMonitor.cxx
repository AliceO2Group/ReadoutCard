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
#include "Cru/Common.h"
#include "Cru/Constants.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
             "roc-pkt-monitor --id 42:00.0\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("json-out",
                          po::bool_switch(&mOptions.jsonOut),
                          "Toggle json-formatted output");
    options.add_options()("csv-out",
                          po::bool_switch(&mOptions.csvOut),
                          "Toggle csv-formatted output");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    auto cardId = Options::getOptionCardId(map);
    auto params = Parameters::makeParameters(cardId, 2); //status available on BAR2
    auto bar2 = ChannelFactory().getBar(params);

    CardType::type cardType = bar2->getCardType();
    if (cardType == CardType::type::Crorc) {
      std::cout << "CRORC packet monitoring not yet supported" << std::endl;
      return;
    } else if (cardType != CardType::type::Cru) {
      std::cout << "Invalid card type" << std::endl;
      return;
    }

    auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar2);

    Cru::PacketMonitoringInfo packetMonitoringInfo = cruBar2->monitorPackets();

    /* HEADER */
    std::ostringstream table;
    auto formatHeader = "  %-9s %-14s %-14s %-12s\n";
    auto formatRow = "  %-9s %-14s %-14s %-12s\n";
    auto header = (boost::format(formatHeader) % "Link ID" % "Accepted" % "Rejected" % "Forced").str();
    auto lineFat = std::string(header.length(), '=') + '\n';
    auto lineThin = std::string(header.length(), '-') + '\n';

    if (mOptions.csvOut) {
      auto csvHeader = "Link ID,Accepted,Rejected,Forced\n";
      std::cout << csvHeader;
    } else if (!mOptions.jsonOut) {
      table << lineFat << header << lineThin;
    }

    // initialize ptrees
    pt::ptree root;
    pt::ptree gbtLinks;
    pt::ptree ulLink;

    /* TABLE */
    for (const auto& el : packetMonitoringInfo.linkPacketInfoMap) {
      int globalId = el.first;
      auto linkMonitoringInfoMap = el.second;
      uint32_t accepted = linkMonitoringInfoMap.accepted;
      uint32_t rejected = linkMonitoringInfoMap.rejected;
      uint32_t forced = linkMonitoringInfoMap.forced;

      /* UL HEADER */
      if (globalId == 15) {
        auto uLHeader = (boost::format(formatHeader) % "ULL ID " % "Accepted" % "Rejected" % "Forced").str();

        if (mOptions.jsonOut) {

        } else if (mOptions.csvOut) {
          auto uLHeader = "Link ID,Accepted,Rejected,Forced\n";
          std::cout << uLHeader;
        } else {
          table << lineFat << uLHeader << lineThin;
        }
      }

      if (mOptions.jsonOut) {
        pt::ptree linkNode;

        // add kv pairs for this link
        linkNode.put("linkId", std::to_string(globalId));
        linkNode.put("accepted", std::to_string(accepted));
        linkNode.put("rejected", std::to_string(rejected));
        linkNode.put("forced", std::to_string(forced));

        // append to the links (or UL link)
        if (globalId == 15) {
          ulLink.add_child(std::to_string(globalId), linkNode);
        } else {
          gbtLinks.add_child(std::to_string(globalId), linkNode);
        }
      } else if (mOptions.csvOut) {
        auto csvLine = std::to_string(globalId) + "," + std::to_string(accepted) + "," + std::to_string(rejected) + "," + std::to_string(forced) + "\n";
        std::cout << csvLine;
      } else {
        auto format = boost::format(formatRow) % globalId % accepted % rejected % forced;
        table << format;
      }
    }

    // add links nodes to the tree
    root.add_child("gbtLinks", gbtLinks);
    root.add_child("userLogicLink", ulLink);

    /* PRINT */
    if (!mOptions.csvOut) {
      std::cout << table.str();
    }

    /* HEADER */
    std::ostringstream otherTable;
    formatHeader = "  %-9s %-16s %-25s\n";
    formatRow = "  %-9s %-16s %-25s\n";
    header = (boost::format(formatHeader) % "Wrapper" % "Dropped" % "Total Packets per second").str();
    lineFat = std::string(header.length(), '=') + '\n';
    lineThin = std::string(header.length(), '-') + '\n';

    if (mOptions.csvOut) {
      auto csvHeader = "Wrapper,Dropped,Total Packets per second\n";
      std::cout << csvHeader;
    } else {
      otherTable << lineFat << header << lineThin;
    }

    pt::ptree ulLinks;

    /* TABLE */
    for (const auto& el : packetMonitoringInfo.wrapperPacketInfoMap) {
      int wrapper = el.first;
      auto wrapperMonitoringInfoMap = el.second;
      uint32_t dropped = wrapperMonitoringInfoMap.dropped;
      uint32_t totalPacketsPerSec = wrapperMonitoringInfoMap.totalPacketsPerSec;

      if (mOptions.jsonOut) {
        pt::ptree wrapperNode;

        // add kv pairs for this wrapper
        wrapperNode.put("wrapperId", std::to_string(wrapper));
        wrapperNode.put("dropped", std::to_string(dropped));
        wrapperNode.put("totalPacketsPerSec", std::to_string(totalPacketsPerSec));

        // add the wrapper node to the tree
        root.add_child("wrapper", wrapperNode);
      }
      if (mOptions.csvOut) {
        auto csvLine = std::to_string(wrapper) + "," + std::to_string(dropped) + "," + std::to_string(totalPacketsPerSec) + "\n";
        std::cout << csvLine;
      } else {
        auto format = boost::format(formatRow) % wrapper % dropped % totalPacketsPerSec;
        otherTable << format;
      }
    }

    /* BREAK + PRINT */
    if (mOptions.jsonOut) {
      pt::write_json(std::cout, root);
    } else if (!mOptions.csvOut) {
      auto lineFat = std::string(header.length(), '=') + '\n';
      otherTable << lineFat;
      std::cout << otherTable.str();
    }
  }

 private:
  struct OptionsStruct {
    bool jsonOut = false;
    bool csvOut = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramPacketMonitor().execute(argc, argv);
}
