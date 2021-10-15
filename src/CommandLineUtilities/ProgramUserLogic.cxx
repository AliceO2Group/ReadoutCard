
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
/// \file ProgramUserLogic.cxx
/// \brief Tool to control and report on the dummy(!) User Logic
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Cru/CruBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"

using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
namespace po = boost::program_options;

class ProgramUserLogic : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "User Logic", "Control the dummy User Logic",
             "o2-roc-ul --id 0042:0 --event-size=128 \n"
             "o2-roc-ul --id 0042:0 --random-event-size \n"
             "o2-roc-ul --id 0042:0 --status \n" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    Options::addOptionCardId(options);
    options.add_options()("random-event-size",
                          po::bool_switch(&mOptions.randomEventSize),
                          "Toggle random event size");
    options.add_options()("event-size",
                          po::value<uint32_t>(&mOptions.eventSize)->default_value(100),
                          "Set the event size (in GBT words = 128bits)");
    options.add_options()("status",
                          po::bool_switch(&mOptions.status),
                          "Print UL status only");
    options.add_options()("system-id",
                          po::value<uint32_t>(&mOptions.systemId)->default_value(0xff),
                          "Set the System ID");
    options.add_options()("link-id",
                          po::value<uint32_t>(&mOptions.linkId)->default_value(0xf),
                          "Set the Link ID");
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    auto cardId = Options::getOptionCardId(map);
    auto card = RocPciDevice(cardId).getCardDescriptor();
    auto cardType = card.cardType;
    if (cardType != CardType::type::Cru) {
      std::cerr << "Unsupported card type, only CRU supported." << std::endl;
      return;
    }

    Parameters params = Parameters::makeParameters(cardId, 2);
    auto bar = ChannelFactory().getBar(params);
    auto cruBar2 = std::dynamic_pointer_cast<CruBar>(bar);
    if (mOptions.status) {
      Cru::UserLogicInfo ulInfo = cruBar2->reportUserLogic();
      std::cout << "==========================" << std::endl;
      std::cout << "System ID : 0x" << std::hex << ulInfo.systemId << std::endl;
      std::cout << "Link ID   : " << std::dec << ulInfo.linkId << std::endl;
      std::cout << "Event size: " << ulInfo.eventSize << " GBT words" << std::endl;
      std::cout << "Event size: " << (ulInfo.eventSize * 128) / 1024.0 << "Kb" << std::endl;
      std::cout << "Event size: " << (ulInfo.eventSize * 128) / (1024.0 * 8) << "KB" << std::endl;
      std::cout << "Randomized: " << std::boolalpha << ulInfo.random << std::endl;
      std::cout << "==========================" << std::endl;
    } else {
      cruBar2->controlUserLogic(mOptions.eventSize, mOptions.randomEventSize, mOptions.systemId, mOptions.linkId);
    }
  }

 private:
  struct OptionsStruct {
    uint32_t eventSize;
    bool randomEventSize;
    bool status;
    uint32_t systemId;
    uint32_t linkId;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramUserLogic().execute(argc, argv);
}
