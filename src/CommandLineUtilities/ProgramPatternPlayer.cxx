 
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

// 128-bit hex string parser
// nBits specifies the maximum allowed bit width
// throws exception on error
uint128_t parsePattern(const std::string &s, int nBits = 128) {
  uint128_t v = 0;
  if (strncmp(s.c_str(), "0x", 2) || (s.length() <= 2)) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
         << ErrorInfo::Message("Pattern must be hexadecimal"));
  }
  for (unsigned int i=2; i<s.length(); i++) {
    int c = s[i];
    uint128_t x = 0;
    if ( ( c >= '0') && ( c <= '9')) {
      x = c - '0';
    } else if ( ( c >= 'A') && ( c <= 'F')) {
      x = 10 + c - 'A';
    } else if ( ( c >= 'a') && ( c <= 'f')) {
      x = 10 + c - 'a';
    } else {
      BOOST_THROW_EXCEPTION(InvalidOptionValueException()
         << ErrorInfo::Message("Pattern must be hexadecimal"));
    }
    v = (v << 4) + x;

    /*
    // print value
    for (int k=8; k > 0; k--) {
      printf("%04X ", (unsigned int)((v >> ((k-1)*16)) & 0xffff));
    }
    printf("\n");
    */
  }

  if (((v >> nBits) != 0) || (s.length() - 2 > 32)) {
    BOOST_THROW_EXCEPTION(InvalidOptionValueException()
                      << ErrorInfo::Message("Pattern exceeds " + std::to_string(nBits) + " bits"));
  }
  return v;
}

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
    options.add_options()("pat0",
                          po::value<std::string>(&mOptions.pat0),
                          "80-bit pat0 pattern in hex");
    options.add_options()("pat1",
                          po::value<std::string>(&mOptions.pat1),
                          "80-bit pat1 pattern in hex");
    options.add_options()("pat2",
                          po::value<std::string>(&mOptions.pat2),
                          "80-bit pat2 pattern in hex");
    options.add_options()("pat3",
                          po::value<std::string>(&mOptions.pat3),
                          "80-bit pat3 pattern in hex");
    options.add_options()("pat1-length",
                          po::value<uint32_t>(&mOptions.info.pat1Length),
                          "pat1 pattern's length");
    options.add_options()("pat1-delay",
                          po::value<uint32_t>(&mOptions.info.pat1Delay),
                          "pat1 pattern's delay");
    options.add_options()("pat2-length",
                          po::value<uint32_t>(&mOptions.info.pat2Length),
                          "pat2 pattern's length");
    options.add_options()("pat2-trigger-counters",
                          po::value<uint32_t>(&mOptions.info.pat2TriggerTF),
                          "Trigger counters for pat2: TF[31:20] ORBIT[19:12] BC[11:0]");
    options.add_options()("pat3-length",
                          po::value<uint32_t>(&mOptions.info.pat3Length),
                          "pat3 pattern's length");
    options.add_options()("pat1-trigger-select",
                          po::value<uint32_t>(&mOptions.info.pat1TriggerSelect),
                          "Select trigger for pat1");
    options.add_options()("pat2-trigger-select",
                          po::value<uint32_t>(&mOptions.info.pat2TriggerSelect),
                          "Select trigger for pat2");
    options.add_options()("pat3-trigger-select",
                          po::value<uint32_t>(&mOptions.info.pat3TriggerSelect),
                          "Select trigger for pat3");
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

    if (mOptions.pat0.length()) {
      mOptions.info.pat0 = parsePattern(mOptions.pat0, 80);
    }
    if (mOptions.pat1.length()) {
      mOptions.info.pat1 = parsePattern(mOptions.pat1, 80);
    }
    if (mOptions.pat2.length()) {
      mOptions.info.pat2 = parsePattern(mOptions.pat2, 80);
    }
    if (mOptions.pat3.length()) {
      mOptions.info.pat3 = parsePattern(mOptions.pat3, 80);
    }

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
    std::string pat0;
    std::string pat1;
    std::string pat2;
    std::string pat3;
    PatternPlayer::Info info;
    bool readBack = false;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramPatternPlayer().execute(argc, argv);
}
