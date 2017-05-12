/// \file ProgramReset.cxx
/// \brief Utility that resets a ReadoutCard
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iostream>
#include "ReadoutCard/ChannelFactory.h"

namespace {
using namespace AliceO2::roc::CommandLineUtilities;

class ProgramReset: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"Reset", "Resets a channel", "roc-reset --id=12345 --channel=0 --reset=INTERNAL_DIU_SIU"};
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionCardId(options);
      Options::addOptionResetLevel(options);
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      auto resetLevel = Options::getOptionResetLevel(map);
      auto cardId = Options::getOptionCardId(map);
      int channelNumber = Options::getOptionChannel(map);

      auto params = AliceO2::roc::Parameters::makeParameters(cardId, channelNumber);
      auto channel = AliceO2::roc::ChannelFactory().getDmaChannel(params);
      channel->resetChannel(resetLevel);
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramReset().execute(argc, argv);
}
