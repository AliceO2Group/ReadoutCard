///
/// \file RorcUtilsReadRegister.cxx
/// \author Pascal Boeschoten
///
/// Utility that blinks the CRU LED
///

#include <iostream>
#include <thread>
#include <chrono>
#include "RORC/ChannelFactory.h"
#include "RORC/CardType.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include "RorcException.h"

using namespace AliceO2::Rorc::Util;

class ProgramCruBlink: public RorcUtilsProgram
{
  public:
    virtual ~ProgramCruBlink()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("CRU Blink", "Blinks the CRU LED", "./rorc-cru-blink");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionSerialNumber(options);
    }

    virtual void mainFunction(boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int channelNumber = 0;
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      if (channel->getCardType() == AliceO2::Rorc::CardType::CRU) {
        ALICEO2_RORC_THROW_EXCEPTION("Card is not a CRU");
      }

      const int LED_REGISTER_INDEX = 152;
      bool turnOn = true;
      for (int i = 0; i < 10; ++i) {
        if (isSigInt()) {
          std::cout << "\nInterrupted - Turning LED off" << std::endl;
          channel->writeRegister(LED_REGISTER_INDEX, 0);
          break;
        }

        std::cout << (turnOn ? "On" : "Off") << std::endl;
        channel->writeRegister(LED_REGISTER_INDEX, (turnOn ? 1 : 0));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        turnOn = !turnOn;
      }
    }
};

int main(int argc, char** argv)
{
  return ProgramCruBlink().execute(argc, argv);
}
