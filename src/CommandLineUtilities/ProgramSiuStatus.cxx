// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramSiuStatus: public Program
{
  public:

  virtual Description getDescription()
  {
    return {"SIU Status", "Report SIU Status", 
      "roc-siu-status --id=42:00.0 --channel=2"};
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()
      ("channel",
       po::value<int>(&mOptions.channel)->default_value(0),
       "Channel (0-5)");
    Options::addOptionCardId(options);
  }

  virtual void run(const boost::program_options::variables_map& map) 
  {
    if ((mOptions.channel < 0) || (mOptions.channel > 5)) {
      getLogger() << "Please provide a channel in the 0-5 range." << InfoLogger::endm;
      return;
    }
    auto cardId = Options::getOptionCardId(map);
    getLogger() << "Card ID: " << cardId << InfoLogger::endm;
    getLogger() << "Channel: " << mOptions.channel << InfoLogger::endm;

    std::shared_ptr<BarInterface>
      bar = ChannelFactory().getBar(cardId, mOptions.channel);

    auto cardType = bar->getCardType();
    if (cardType != CardType::Crorc) {
      getLogger() << InfoLogger::Warning <<
        "SIU status only applicable to CRORC" << InfoLogger::endm;
      return;
    }

    Crorc::Crorc crorc = Crorc::Crorc(*(bar.get()));
    std::tuple<std::string, uint32_t> siuStatus;
    try {
      siuStatus = crorc.siuStatus();
    } catch (const Exception& e){
      getLogger() << InfoLogger::Error <<
        diagnostic_information(e) << InfoLogger::endm;
      return;
    }

    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::hex << std::get<1>(siuStatus);

    getLogger() << "SIU HW info: " << std::get<0>(siuStatus) << InfoLogger::endm;
    getLogger() << "SIU Status Register: " << ss.str() << InfoLogger::endm;
    auto statusStrings = crorc.ddlInterpretIfstw(std::get<1>(siuStatus));

    for(auto const& string: statusStrings) {
      getLogger() << string << InfoLogger::endm;
    }
  }

  private:
  struct OptionsStruct 
  {
    int channel;
  }mOptions;
};

int main(int argc, char** argv)
{
  return ProgramSiuStatus().execute(argc, argv);
}
