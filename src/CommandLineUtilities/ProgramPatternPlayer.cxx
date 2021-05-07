// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramPatternPlayer.cxx
/// \brief Tool to use the pattern player
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>

#include "Cru/Common.h"
#include "Cru/Constants.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/PatternPlayer.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Utilities/Enum.h"
#include <boost/format.hpp>
#include <boost/multiprecision/cpp_int.hpp>

using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
namespace po = boost::program_options;
using namespace boost::multiprecision;

class ProgramPatternPlayer : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "PatternPlayer", "Configure the CRU pattern player",
             "o2-roc-pat-player --id 42:00.0 --sync 0x0123457899876543210abcdefedcb --sync-length 4 --sync-delay 2 --sync-at-start\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("sync",
                          po::value<std::string>(&mOptions.syncPattern)->default_value("0x0"),
                          "80-bit sync pattern in hex");
    options.add_options()("reset",
                          po::value<std::string>(&mOptions.resetPattern)->default_value("0x0"),
                          "80-bit reset pattern in hex");
    options.add_options()("idle",
                          po::value<std::string>(&mOptions.idlePattern)->default_value("0x0"),
                          "80-bit idle pattern in hex");
    options.add_options()("sync-length",
                          po::value<uint32_t>(&mOptions.syncLength)->default_value(1),
                          "Sync pattern's length");
    options.add_options()("sync-delay",
                          po::value<uint32_t>(&mOptions.syncDelay)->default_value(0),
                          "Sync pattern's delay");
    options.add_options()("reset-length",
                          po::value<uint32_t>(&mOptions.resetLength)->default_value(1),
                          "Reset pattern's length");
    options.add_options()("reset-trigger-select",
                          po::value<uint32_t>(&mOptions.resetTriggerSelect)->default_value(30),
                          "Select trigger for SYNC from TTC_DATA[0-31]");
    options.add_options()("sync-trigger-select",
                          po::value<uint32_t>(&mOptions.syncTriggerSelect)->default_value(29),
                          "Select trigger for RESET from TTC_DATA[0-31]");
    options.add_options()("sync-at-start",
                          po::bool_switch(&mOptions.syncAtStart)->default_value(false),
                          "Enable automatically sending a sync pattern when runenable goes high");
    options.add_options()("trigger-sync",
                          po::bool_switch(&mOptions.triggerSync)->default_value(false),
                          "Manually trigger the SYNC pattern");
    options.add_options()("trigger-reset",
                          po::bool_switch(&mOptions.triggerReset)->default_value(false),
                          "Manually trigger the reset pattern");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    auto cardId = Options::getOptionCardId(map);
    auto params = Parameters::makeParameters(cardId, 2);
    auto bar2 = ChannelFactory().getBar(params);

    CardType::type cardType = bar2->getCardType();
    if (cardType == CardType::type::Crorc) {
      std::cout << "CRORC not supported" << std::endl;
      return;
    } else if (cardType != CardType::type::Cru) {
      std::cout << "Invalid card type" << std::endl;
      return;
    }

    auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar2);
    cruBar2->patternPlayer({ uint128_t(mOptions.syncPattern), //TODO: Parse this correctly!
                             uint128_t(mOptions.resetPattern),
                             uint128_t(mOptions.idlePattern),
                             mOptions.syncLength,
                             mOptions.syncDelay,
                             mOptions.resetLength,
                             mOptions.resetTriggerSelect,
                             mOptions.syncTriggerSelect,
                             mOptions.syncAtStart,
                             mOptions.triggerSync,
                             mOptions.triggerReset });
  }

 private:
  struct OptionsStruct {
    std::string syncPattern = "0x0";
    std::string resetPattern = "0x0";
    std::string idlePattern = "0x0";
    uint32_t syncLength = 1;
    uint32_t syncDelay = 0;
    uint32_t resetLength = 1;
    uint32_t resetTriggerSelect = 30;
    uint32_t syncTriggerSelect = 29;
    bool syncAtStart = false;
    bool triggerSync = false;
    bool triggerReset = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramPatternPlayer().execute(argc, argv);
}
