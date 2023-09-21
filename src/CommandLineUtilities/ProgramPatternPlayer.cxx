 
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
             "o2-roc-pat-player --id 42:00.0 --pat1 0x012345789abdef0123 --pat1-length 4 --pat1-delay 2 --execute-pat1-at-start\n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);

    optionValues.reserve(16); // need to keep same variable addresses

    addOptionValue(options, "pat0", "80-bit pat0 pattern", &mOptions.info.pat0, 80);
    addOptionValue(options, "pat1", "80-bit pat1 pattern", &mOptions.info.pat1, 80);
    addOptionValue(options, "pat2", "80-bit pat2 pattern", &mOptions.info.pat2, 80);
    addOptionValue(options, "pat3", "80-bit pat3 pattern", &mOptions.info.pat3, 80);
    addOptionValue(options, "pat1-length", "pat1 pattern's length", &mOptions.info.pat1Length);
    addOptionValue(options, "pat1-delay", "pat1 pattern's delay", &mOptions.info.pat1Delay);
    addOptionValue(options, "pat2-length", "pat2 pattern's length", &mOptions.info.pat2Length);
    addOptionValue(options, "pat2-trigger-counters", "Trigger counters for pat2: TF[31:20] ORBIT[19:12] BC[11:0]", &mOptions.info.pat2TriggerTF);
    addOptionValue(options, "pat3-length", "pat3 pattern's length", &mOptions.info.pat3Length);
    addOptionValue(options, "pat1-trigger-select", "Select trigger for pat1", &mOptions.info.pat1TriggerSelect);
    addOptionValue(options, "pat2-trigger-select", "Select trigger for pat2", &mOptions.info.pat2TriggerSelect);
    addOptionValue(options, "pat3-trigger-select", "Select trigger for pat3", &mOptions.info.pat3TriggerSelect);

    options.add_options()("execute-pat1-at-start",
                          po::bool_switch(&mOptions.info.exePat1AtStart)->default_value(false),
                          "Enable automatically sending a pat1 pattern when runenable goes high");
    options.add_options()("execute-pat1-now",
                          po::bool_switch(&mOptions.info.exePat1Now)->default_value(false),
                          "Manually trigger the pat1 pattern now");
    options.add_options()("execute-pat2-now",
                          po::bool_switch(&mOptions.info.exePat2Now)->default_value(false),
                          "Manually trigger the pat2 pattern now");
    options.add_options()("read-back",
                          po::bool_switch(&mOptions.readBack)->default_value(false),
                          "Reads back the pattern player configuration [DOES NOT CONFIGURE!!]");
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

    // convert input strings to numbers
    parseOptionValues();

    auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar2);
    if (!mOptions.readBack) {
      cruBar2->patternPlayer(mOptions.info);
    } else {
      auto ppInfo = cruBar2->patternPlayerRead();
      std::cout << "pat0 pattern:\t\t0x" << std::hex << ppInfo.pat0 << std::endl;
      std::cout << "pat1 pattern:\t\t0x" << std::hex << ppInfo.pat1 << std::endl;
      std::cout << "pat2 pattern:\t\t0x" << std::hex << ppInfo.pat2 << std::endl;
      std::cout << "pat3 pattern:\t\t0x" << std::hex << ppInfo.pat3 << std::endl;

      std::cout << "pat1 length:\t\t" << std::dec << ppInfo.pat1Length << std::endl;
      std::cout << "pat1 delay:\t\t" << std::dec << ppInfo.pat1Delay << std::endl;
      std::cout << "pat2 length:\t\t" << std::dec << ppInfo.pat2Length << std::endl;
      std::cout << "pat3 length:\t\t" << std::dec << ppInfo.pat3Length << std::endl;

      std::cout << "pat1 trigger select:\t0x" << std::hex << ppInfo.pat1TriggerSelect << std::endl;
      std::cout << "pat2 trigger select:\t0x" << std::hex << ppInfo.pat2TriggerSelect << std::endl;
      std::cout << "pat3 trigger select:\t0x" << std::hex << ppInfo.pat3TriggerSelect << std::endl;

      std::cout << "pat2 trigger counters:\t"
        << "TF 0x" << std::hex << ((ppInfo.pat2TriggerTF >> 20) & 0xFFF)
        << " ORBIT 0x" << std::hex << ((ppInfo.pat2TriggerTF >> 12) & 0xFF)
        << " BC 0x" << std::hex << (ppInfo.pat2TriggerTF & 0xFFF)
        << std::endl;
    }
  }

 private:
  struct OptionsStruct {
    PatternPlayer::Info info;
    bool readBack = false;
  } mOptions;

  struct OptionValue {
    public:
    enum OptionValueType {UINT32, UINT64, UINT128};
    std::string name; // option name
    std::string value; // string value
    void * destination; // variable assigned when parsing value
    unsigned int bitWidth; // maximum bit width allowed
    OptionValueType type; // variable type
  }; // namespace option

  std::vector<OptionValue> optionValues;

  // register 32bit option value
  void addOptionValue(boost::program_options::options_description& options, const char *name, const char *description, uint32_t *destinationVariable, unsigned int bitWidth = 32) {
    optionValues.push_back({name, "", destinationVariable, bitWidth, OptionValue::OptionValueType::UINT32});
    options.add_options()(name, po::value<std::string>(&(optionValues.back().value)), description);
  }

  // register 128bit option value
  void addOptionValue(boost::program_options::options_description& options, const char *name, const char *description, uint128_t *destinationVariable, unsigned int bitWidth = 128) {
    optionValues.push_back({name, "", destinationVariable, bitWidth, OptionValue::OptionValueType::UINT128});
    options.add_options()(name, po::value<std::string>(&(optionValues.back().value)), description);
  }

  // parse option values and assign corresponding variables
  void parseOptionValues() {
    for(const auto &opt : optionValues) {
      if (opt.value.length()) {
        uint128_t v;
        v = PatternPlayer::getValueFromString(opt.value, opt.bitWidth, opt.name);
        if (opt.type == OptionValue::OptionValueType::UINT32) {
          *(static_cast<uint32_t *>(opt.destination)) = (uint32_t)v;
        } else if (opt.type == OptionValue::OptionValueType::UINT128) {
          *(static_cast<uint128_t *>(opt.destination)) = v;
        }
      }
    }
  }
};

int main(int argc, char** argv)
{
  return ProgramPatternPlayer().execute(argc, argv);
}
