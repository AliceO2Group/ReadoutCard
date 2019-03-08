// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramRegisterWrite.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that writes to a register on a card

#include "CommandLineUtilities/Program.h"
#include "ReadoutCard/ChannelFactory.h"

using namespace AliceO2::roc::CommandLineUtilities;

namespace {

const char* NOREAD_SWITCH("noread");

class ProgramRegisterWrite: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"Write Register", "Write a value to a single register",
          "roc-reg-write --id=12345 --channel=0 --address=0x8 --value=0"};
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionCardId(options);
      Options::addOptionRegisterValue(options);
      options.add_options()(NOREAD_SWITCH, "No readback of register after write");
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      auto cardId = Options::getOptionCardId(map);
      uint32_t address = Options::getOptionRegisterAddress(map);
      int channelNumber = Options::getOptionChannel(map);
      uint32_t registerValue = Options::getOptionRegisterValue(map);
      auto readback = !bool(map.count(NOREAD_SWITCH));
      auto params = AliceO2::roc::Parameters::makeParameters(cardId, channelNumber);
      auto channel = AliceO2::roc::ChannelFactory().getBar(params);

      // Registers are indexed by 32 bits (4 bytes)
      channel->writeRegister(address / 4, registerValue);
      if (readback) {
        auto value = channel->readRegister(address / 4);
        if (isVerbose()) {
          std::cout << Common::makeRegisterString(address, value) << '\n';
        } else {
          std::cout << "0x" << std::hex << value << '\n';
        }
      } else {
        std::cout << (isVerbose() ? "Done!\n" : "\n");
      }
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramRegisterWrite().execute(argc, argv);
}
