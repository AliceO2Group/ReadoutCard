// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramOnuStatus.cxx
/// \brief Tool that returns the ONU status of RoCs.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Cru/CruBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
namespace pt = boost::property_tree;
namespace po = boost::program_options;

class ProgramOnuStatus : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Status", "Return ONU status",
             "roc-onu-status --id 42:00.0\n"
             "roc-onu-status --id 42:00.0 --json" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("json-out",
                          po::bool_switch(&mOptions.jsonOut),
                          "Toggle json-formatted output");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {
    // initialize ptree
    pt::ptree root;

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

    Cru::OnuStatus onuStatus = cruBar2->reportOnuStatus();

    if (mOptions.jsonOut) {
      root.put("ONU address", onuStatus.onuAddress);
      root.put("ONU RX40 locked", onuStatus.rx40Locked);
      root.put("ONU phase good", onuStatus.phaseGood);
      root.put("ONU RX locked", onuStatus.rxLocked);
      root.put("ONU operational", onuStatus.operational);
      root.put("ONU MGT TX ready", onuStatus.mgtTxReady);
      root.put("ONU MGT RX ready", onuStatus.mgtRxReady);
      root.put("ONU MGT TX PLL locked", onuStatus.mgtTxPllLocked);
      root.put("ONU MGT RX PLL locked", onuStatus.mgtRxPllLocked);
    } else {
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
    }

    if (mOptions.jsonOut) {
      pt::write_json(std::cout, root);
    }
  }

 private:
  struct OptionsStruct {
    bool jsonOut = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramOnuStatus().execute(argc, argv);
}
