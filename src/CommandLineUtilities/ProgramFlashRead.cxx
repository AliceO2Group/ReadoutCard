
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
/// \file ProgramFlashRead.cxx
/// \brief Utility that reads the flash
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iostream>
#include <string>
#include "ReadoutCard/ChannelFactory.h"
#include "Crorc/Crorc.h"
#include "ExceptionInternal.h"

using namespace o2::roc::CommandLineUtilities;
using std::cout;
using std::endl;
namespace po = boost::program_options;

namespace
{
class ProgramCrorcFlash : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Flash Read", "Reads card flash memory",
             "o2-roc-flash-read --id=12345 --address=0 --words=32" };
  }

  virtual void addOptions(po::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("address", po::value<uint64_t>(&mAddress)->default_value(0), "Starting address to read");
    options.add_options()("words", po::value<uint64_t>(&mWords)->required(), "Amount of 32-bit words to read");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {
    using namespace o2::roc;

    auto cardId = Options::getOptionCardId(map);
    auto channelNumber = 0;
    auto params = o2::roc::Parameters::makeParameters(cardId, channelNumber);
    auto channel = o2::roc::ChannelFactory().getBar(params);

    if (channel->getCardType() != CardType::Crorc) {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Only C-RORC supported for now"));
    }

    Crorc::readFlashRange(*channel.get(), mAddress, mWords, std::cout);
  }

  uint64_t mAddress = 0;
  uint64_t mWords = 0;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCrorcFlash().execute(argc, argv);
}
