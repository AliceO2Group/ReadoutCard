// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramRegisterRead.cxx
/// \brief Utility that reads a register from a card
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iostream>
#include "ReadoutCard/ChannelFactory.h"

namespace
{
using namespace AliceO2::roc::CommandLineUtilities;

class ProgramRegisterRead : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Read Register", "Read a single register", "roc-reg-read --id=12345 --channel=0 --address=0x8" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionRegisterAddress(options);
    Options::addOptionChannel(options);
    Options::addOptionCardId(options);
  }

  virtual void run(const boost::program_options::variables_map& map)
  {
    auto cardId = Options::getOptionCardId(map);
    uint32_t address = Options::getOptionRegisterAddress(map);
    int channelNumber = Options::getOptionChannel(map);
    auto params = AliceO2::roc::Parameters::makeParameters(cardId, channelNumber);
    auto channel = AliceO2::roc::ChannelFactory().getBar(params);

    // Registers are indexed by 32 bits (4 bytes)
    uint32_t value = channel->readRegister(address / 4);
    if (isVerbose()) {
      std::cout << Common::makeRegisterString(address, value) << '\n';
    } else {
      std::cout << "0x" << std::hex << value << '\n';
    }
  }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramRegisterRead().execute(argc, argv);
}
