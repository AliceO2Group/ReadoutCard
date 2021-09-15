
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
/// \file ProgramSiuStatus.cxx
/// \brief Tool that prints the Status of the SIU
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include "Crorc/Crorc.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"

using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
namespace po = boost::program_options;

class ProgramSiuStatus : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "SIU Status", "Report SIU Status",
             "o2-roc-siu-status --id=42:00.0 --channel=2" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()("channel",
                          po::value<int>(&mOptions.channel)->default_value(0),
                          "Channel (0-5)");
    Options::addOptionCardId(options);
  }

  virtual void run(const boost::program_options::variables_map& map)
  {
    if ((mOptions.channel < 0) || (mOptions.channel > 5)) {
      std::cout << "Please provide a channel in the 0-5 range." << std::endl;
      return;
    }
    auto cardId = Options::getOptionCardId(map);
    std::cout << "Card ID: " << cardId << std::endl;
    std::cout << "Channel: " << mOptions.channel << std::endl;

    std::shared_ptr<BarInterface>
      bar = ChannelFactory().getBar(cardId, mOptions.channel);

    auto cardType = bar->getCardType();
    if (cardType != CardType::Crorc) {
      std::cout << "SIU status only applicable to CRORC" << std::endl;
      return;
    }

    Crorc::Crorc crorc = Crorc::Crorc(*(bar.get()));
    std::tuple<std::string, uint32_t> siuStatus;
    try {
      siuStatus = crorc.siuStatus();
    } catch (const Exception& e) {
      std::cerr << diagnostic_information(e) << std::endl;
      throw(e);
    }

    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::hex << std::get<1>(siuStatus);

    std::cout << "SIU HW info: " << std::get<0>(siuStatus) << std::endl;
    std::cout << "SIU Status Register: " << ss.str() << std::endl;
    auto statusStrings = crorc.ddlInterpretIfstw(std::get<1>(siuStatus));

    for (auto const& string : statusStrings) {
      std::cout << string << std::endl;
    }
  }

 private:
  struct OptionsStruct {
    int channel;
  } mOptions;
};

int main(int argc, char** argv)
{
  return ProgramSiuStatus().execute(argc, argv);
}
