///
/// \file RorcUtilsCruBlink.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that blinks the CRU LED
///

#include <iostream>
#include <thread>
#include <chrono>
#include "ChannelUtilityFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include "RorcException.h"

using namespace AliceO2::Rorc::Util;
using std::cout;
using std::endl;

class ProgramCruBlink: public RorcUtilsProgram
{
  public:
    virtual ~ProgramCruBlink()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("CRU Blink", "Blinks the CRU LED", "./rorc-cru-blink --serial=12345");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionSerialNumber(options);
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
    {
      const int serialNumber = Options::getOptionSerialNumber(map);
      const int channelNumber = 0;
      auto channel = AliceO2::Rorc::ChannelUtilityFactory().getUtility(serialNumber, channelNumber);

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

int main(int argc, char** argv)
{
  return ProgramCruBlink().execute(argc, argv);
}
