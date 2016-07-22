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

    virtual void mainFunction(boost::program_options::variables_map& map)
    {
      const int serialNumber = Options::getOptionSerialNumber(map);
      const int channelNumber = 0;
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      if (channel->getCardType() != AliceO2::Rorc::CardType::CRU) {
        ALICEO2_RORC_THROW_EXCEPTION("Card is not a CRU");
      }

      const int LED_REGISTER_ADDRESS = 0x260;
      const auto cycle = std::chrono::milliseconds(250);
      bool turnOn = true;

      for (int i = 0; i < 1000; ++i) {
        const int index = LED_REGISTER_ADDRESS / 4;
        const int valueOn = 0xff;
        const int valueOff = 0x00;

        if (isSigInt()) {
          std::cout << "\nInterrupted - Turning LED off" << std::endl;
          channel->writeRegister(index, valueOff);
          break;
        }

        channel->writeRegister(index, turnOn ? valueOn : valueOff);
        cout << (turnOn ? "ON  " : "OFF ") << Common::makeRegisterString(index*4, channel->readRegister(index));
        std::this_thread::sleep_for(cycle);
        turnOn = !turnOn;
      }
    }
};

int main(int argc, char** argv)
{
  return ProgramCruBlink().execute(argc, argv);
}
