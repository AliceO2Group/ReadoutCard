// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramCtpEmulator.cxx
/// \brief Tool to emulate CTP functionality
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Cru/Common.h"
#include "Cru/Constants.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Logger.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Utilities/Enum.h"
#include <boost/format.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
namespace po = boost::program_options;

class ProgramCtpEmulator : public Program
{
 public:
  ProgramCtpEmulator(bool ilgEnabled) : Program(ilgEnabled)
  {
  }

  virtual Description getDescription()
  {
    return { "CTP Emulator", "Emulate CTP functionality",
             "roc-ctp-emulator --id 42:00.0 --trigger-mode continuous\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("bcmax",
                          po::value<uint32_t>(&mOptions.bcMax)->default_value(3563),
                          "Sets the maximum Bunch Crossing value");
    options.add_options()("hbmax",
                          po::value<uint32_t>(&mOptions.hbMax)->default_value(255),
                          "Sets the maximum number of Hearteats within a Timeframe");
    options.add_options()("hbdrop",
                          po::value<uint32_t>(&mOptions.hbDrop)->default_value(15000),
                          "Sets the number of Heartbeats to keep");
    options.add_options()("hbkeep",
                          po::value<uint32_t>(&mOptions.hbKeep)->default_value(15000),
                          "Sets the number Heartbeats to drop");
    options.add_options()("init-orbit",
                          po::value<uint32_t>(&mOptions.orbitInit)->default_value(0),
                          "Sets the initial orbit id");
    options.add_options()("trigger-mode",
                          po::value<std::string>(&mOptions.triggerModeString)->default_value("periodic"),
                          "Sets the trigger mode. Options are periodic, manual, continuous and fixed");
    options.add_options()("trigger-freq",
                          po::value<uint32_t>(&mOptions.triggerFrequency)->default_value(8),
                          "Sets the physics trigger frequency.");
    options.add_options()("eox",
                          po::bool_switch(&mOptions.generateEox)->default_value(false),
                          "Generate an EOX trigger.");
    options.add_options()("single-trigger",
                          po::bool_switch(&mOptions.generateSingleTrigger)->default_value(false),
                          "Generate a single PHY trigger.");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {
    Logger::setFacility("ReadoutCard/CTP emulator");

    auto cardId = Options::getOptionCardId(map);
    auto params = Parameters::makeParameters(cardId, 2);
    auto bar2 = ChannelFactory().getBar(params);

    CardType::type cardType = bar2->getCardType();
    if (cardType == CardType::type::Crorc) {
      Logger::get() << "CRORC not supported" << LogErrorOps << endm;
      return;
    } else if (cardType != CardType::type::Cru) {
      Logger::get() << "Invalid card type" << LogErrorOps << endm;
      return;
    }

    Cru::TriggerMode triggerMode;

    static const auto converter = Utilities::makeEnumConverter<Cru::TriggerMode>("TriggerMode", {
                                                                                                  { Cru::TriggerMode::Manual, "MANUAL" },
                                                                                                  { Cru::TriggerMode::Periodic, "PERIODIC" },
                                                                                                  { Cru::TriggerMode::Continuous, "CONTINUOUS" },
                                                                                                  { Cru::TriggerMode::Fixed, "FIXED" },
                                                                                                  { Cru::TriggerMode::Hc, "HC" },
                                                                                                  { Cru::TriggerMode::Cal, "CAL" },
                                                                                                });

    triggerMode = converter.fromString(mOptions.triggerModeString);

    auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar2);
    cruBar2->emulateCtp({ mOptions.bcMax,
                          mOptions.hbDrop,
                          mOptions.hbKeep,
                          mOptions.hbMax,
                          triggerMode,
                          mOptions.triggerFrequency,
                          mOptions.generateEox,
                          mOptions.generateSingleTrigger,
                          mOptions.orbitInit });
  }

 private:
  struct OptionsStruct {
    uint32_t bcMax = 3563;
    uint32_t hbDrop = 15000;
    uint32_t hbKeep = 15000;
    uint32_t hbMax = 255;
    uint32_t orbitInit = 0;

    std::string triggerModeString = "periodic";
    uint32_t triggerFrequency = 8;

    bool generateEox = false;
    bool generateSingleTrigger = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  // true here enables InfoLogger output by default
  // see the Program constructor
  return ProgramCtpEmulator(true).execute(argc, argv);
}
