/// \file ProgramReset.cxx
/// \brief Utility that resets a RORC
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Utilities/Program.h"
#include <iostream>
#include "RORC/ChannelFactory.h"

namespace {
using namespace AliceO2::Rorc::Utilities;

class ProgramRegisterRead: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return {"Reset", "Resets a channel", "./rorc-reset --id=12345 --channel=0 --reset=RORC_DIU_SIU"};
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

      auto params = AliceO2::Rorc::Parameters::makeParameters(cardId, channelNumber);
      auto channel = AliceO2::Rorc::ChannelFactory().getMaster(params);
      channel->resetChannel(resetLevel);
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramRegisterRead().execute(argc, argv);
}
