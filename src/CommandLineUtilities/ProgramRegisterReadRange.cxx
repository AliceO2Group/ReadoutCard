// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramRegisterReadRange.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that reads a range of registers from a card

#include "CommandLineUtilities/Program.h"
#include "ReadoutCard/ChannelFactory.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace o2::roc::CommandLineUtilities;
namespace po = boost::program_options;

namespace
{
class ProgramRegisterReadRange : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Read Register Range", "Read a range of registers",
             "o2-roc-reg-read-range --id=12345 --channel=0 --address=0x8 --range=10" };
  }

  virtual void addOptions(po::options_description& options)
  {
    Options::addOptionRegisterAddress(options);
    Options::addOptionChannel(options);
    Options::addOptionCardId(options);
    Options::addOptionRegisterRange(options);
    options.add_options()("file", po::value<std::string>(&mFile), "Output to given file in binary format");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {
    auto cardId = Options::getOptionCardId(map);
    int baseAddress = Options::getOptionRegisterAddress(map);
    int channelNumber = Options::getOptionChannel(map);
    int range = Options::getOptionRegisterRange(map);
    auto params = o2::roc::Parameters::makeParameters(cardId, channelNumber);
    auto channel = o2::roc::ChannelFactory().getBar(params);

    std::vector<uint32_t> values(range);

    // Registers are indexed by 32 bits (4 bytes)
    int baseIndex = baseAddress / 4;

    for (int i = 0; i < range; ++i) {
      values[i] = channel->readRegister(baseIndex + i);
    }

    if (mFile.empty()) {
      for (int i = 0; i < range; ++i) {
        std::cout << Common::makeRegisterString((baseIndex + i) * 4, values[i]) << '\n';
      }
    } else {
      std::ofstream stream(mFile, std::ios_base::out);
      stream.write(reinterpret_cast<char*>(values.data()), values.size() * sizeof(decltype(values)::value_type));
    }
  }

 private:
  std::string mFile;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramRegisterReadRange().execute(argc, argv);
}
