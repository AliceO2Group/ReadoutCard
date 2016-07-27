///
/// \file RorcUtilsSanityCheck.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that performs some basic sanity checks
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
#include <boost/format.hpp>

using namespace AliceO2::Rorc::Util;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

namespace b = boost;

class ProgramSanityCheck: public RorcUtilsProgram
{
  public:
    virtual ~ProgramSanityCheck()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Sanity Check", "Does some basic sanity checks on the card",
          "./rorc-sanity-check --serial=12345");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionSerialNumber(options);
      Options::addOptionChannel(options);
    }

    virtual void mainFunction(boost::program_options::variables_map& map)
    {
      const int serialNumber = Options::getOptionSerialNumber(map);
      const int channelNumber = Options::getOptionChannel(map);

      cout << "Warning: if the CRU is in a bad state, this program may result in a crash and reboot of the host\n";
      cout << "  To proceed, type 'y'\n";
      cout << "  To abort, type anything else or give SIGINT (usually Ctrl-c)\n";
      int c = getchar();
      if (c != 'y' || isSigInt()) {
        return;
      }

      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      if (channel->getCardType() != AliceO2::Rorc::CardType::CRU) {
        ALICEO2_RORC_THROW_EXCEPTION("This utility currently only supports CRU card types");
      }

      const auto formatFailure = " [FAILURE] wrote 0x%x @ 0x%x, read 0x%x @ 0x%x";
      const auto formatSuccess = " [success] wrote 0x%x @ 0x%x, read 0x%x @ 0x%x";
      auto printHeader = [&](const std::string& title) {
        static int i = 1;
        cout << b::str(b::format("%d. %s") % i % title) << endl;
        i++;
      };

      {
        // Writing to the LED register has been known to crash and reboot the machine if the CRU is in a bad state.
        // That makes it a good part of this sanity test!

        auto writeCheckLed = [&](int writeValue) {
          const int ledAddress = 0x260;
          const int ledIndex = ledAddress / 4;

          channel->writeRegister(ledIndex, writeValue);
          int readValue = channel->readRegister(ledIndex);
          auto format = (readValue == writeValue) ? formatSuccess : formatFailure;
          cout << b::str(b::format(format) % writeValue % ledAddress % readValue % ledAddress) << endl;
        };

        printHeader("Turning LEDs on");
        writeCheckLed(0xff);
        printHeader("Turning LEDs off");
        writeCheckLed(0x00);
      }

      {
        // The CRU has a little debug register FIFO thing that we can use to check if simple register writing and
        // reading is working properly.
        // We should be able to push values by writing to 'addressPush' and pop by reading from 'addressPop'

        const int addressPush = 0x274;
        const int addressPop = 0x270;
        const int indexPush = addressPush / 4;
        const int indexPop = addressPop / 4;
        const int valuesToPush = 4;
        auto getValue = [](int i) { return 1 << i; };

        printHeader("Debug FIFO");
        for (int i = 0; i < valuesToPush; ++i) {
          channel->writeRegister(indexPush, getValue(i));
        }

        for (int i = 0; i < valuesToPush; ++i) {
          int expectedValue = getValue(i);
          int readValue = channel->readRegister(indexPop);
          auto format = (readValue == expectedValue) ? formatSuccess : formatFailure;
          cout << b::str(b::format(format) % expectedValue % addressPush % readValue % addressPop) << endl;
        }
      }
    }
};

int main(int argc, char** argv)
{
  return ProgramSanityCheck().execute(argc, argv);
}
