
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
/// \file ProgramRegisterModify.cxx
/// \brief Utility that modifies bits of a register on a card
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "CommandLineUtilities/Program.h"
#include "ReadoutCard/ChannelFactory.h"

using namespace o2::roc::CommandLineUtilities;
namespace po = boost::program_options;

const char* NOREAD_SWITCH("noread");

class ProgramRegisterModify : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Modify Register", "Modify bits of a single register",
             "o2-roc-reg-modify --id=12345 --channel=0 --address=0x8 --position=3 --width=1 --value=0" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionRegisterAddress(options);
    Options::addOptionChannel(options);
    Options::addOptionCardId(options);
    Options::addOptionRegisterValue(options);
    options.add_options()("position",
                          po::value<int>(&mOptions.position)->default_value(0),
                          "Position to modify bits on");
    options.add_options()("width",
                          po::value<int>(&mOptions.width)->default_value(1),
                          "Number of bits to modify");
    options.add_options()(NOREAD_SWITCH,
                          "No readback of register before and after write");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {
    auto cardId = Options::getOptionCardId(map);
    uint32_t address = Options::getOptionRegisterAddress(map);
    int channelNumber = Options::getOptionChannel(map);
    uint32_t registerValue = Options::getOptionRegisterValue(map);
    int position = mOptions.position;
    int width = mOptions.width;
    auto readback = !bool(map.count(NOREAD_SWITCH));
    auto params = o2::roc::Parameters::makeParameters(cardId, channelNumber);
    auto channel = o2::roc::ChannelFactory().getBar(params);

    // Registers are indexed by 32 bits (4 bytes)
    if (readback) {
      auto value = channel->readRegister(address / 4);
      if (isVerbose()) {
        std::cout << "Before modification:" << std::endl;
        std::cout << Common::makeRegisterString(address, value) << '\n';
      } else {
        std::cout << "0x" << std::hex << value << '\n';
      }
    }

    channel->modifyRegister(address / 4, position, width, registerValue);

    if (readback) {
      auto value = channel->readRegister(address / 4);
      if (isVerbose()) {
        std::cout << "After modification:" << std::endl;
        std::cout << Common::makeRegisterString(address, value) << '\n';
      } else {
        std::cout << "0x" << std::hex << value << '\n';
      }
    } else {
      std::cout << (isVerbose() ? "Done!\n" : "\n");
    }
  }

  struct OptionsStruct {
    int position = 0;
    int width = 1;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramRegisterModify().execute(argc, argv);
}
