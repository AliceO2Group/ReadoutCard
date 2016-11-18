/// \file ProgramCruBlink.cxx
/// \brief Utility that blinks the CRU LED
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Utilities/Program.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "Factory/ChannelUtilityFactory.h"
#include "RORC/ChannelFactory.h"
#include "RORC/Exception.h"

using namespace AliceO2::Rorc::Utilities;
using std::cout;
using std::endl;

namespace {
class ProgramCruBlink: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return {"CRU Blink", "Blinks the CRU LED", "./rorc-cru-blink --id=12345"};
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionCardId(options);
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      using namespace AliceO2::Rorc;

      auto cardId = Options::getOptionCardId(map);
      auto channelNumber = 0;
      auto params = AliceO2::Rorc::Parameters::makeParameters(cardId, channelNumber);
      auto channel = AliceO2::Rorc::ChannelUtilityFactory().getUtility(params);

      const auto cycle = std::chrono::milliseconds(250);
      bool nextLedState = true;

      for (int i = 0; i < 1000; ++i) {
        if (isSigInt()) {
          std::cout << "\nInterrupted - Turning LED off" << std::endl;
          channel->utilitySetLedState(false);
          break;
        }
        channel->utilitySetLedState(nextLedState);
        cout << (nextLedState ? "ON" : "OFF") << endl;
        std::this_thread::sleep_for(cycle);
        nextLedState = !nextLedState;
      }
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruBlink().execute(argc, argv);
}
