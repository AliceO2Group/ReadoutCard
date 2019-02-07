/// \file ProgramSiuStatus.cxx
/// \brief Tool that prints the Status of the SIU
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <thread>
#include "Crorc/Crorc.h"
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
    //TODO: Check for CRU

    std::shared_ptr<BarInterface>
      bar = ChannelFactory().getBar(cardId, mOptions.channel);

    Crorc::Crorc crorc = Crorc::Crorc(*(bar.get()));
    auto siuStatus = crorc.siuStatus();

    getLogger() << "SIU HW info: " << std::get<0>(siuStatus) << InfoLogger::endm;
    getLogger() << "SIU Status Register: 0x" << std::setfill('0') << std::setw(8) << std::hex << std::get<1>(siuStatus) << InfoLogger::endm;
  }
  //TODO: Explain status register

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
