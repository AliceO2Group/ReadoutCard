///
/// \file RorcUtilsWriteRegister.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that writes to a register on a RORC
///

#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"

using namespace AliceO2::Rorc::Util;

namespace {

const char* NOREAD_SWITCH("noread");

class ProgramWriteRegister: public RorcUtilsProgram
{
  public:
    virtual ~ProgramWriteRegister()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Write Register", "Write a value to a single register",
          "./rorc-reg-write --serial=12345 --channel=0 -a0x8 -v0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      Options::addOptionRegisterValue(options);
      options.add_options()(NOREAD_SWITCH, "No readback of register after write");
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int address = Options::getOptionRegisterAddress(map);
      int channelNumber = Options::getOptionChannel(map);
      int registerValue = Options::getOptionRegisterValue(map);
      auto readback = !bool(map.count(NOREAD_SWITCH));
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      // Registers are indexed by 32 bits (4 bytes)
      channel->writeRegister(address / 4, registerValue);
      if (readback) {
        std::cout << Common::makeRegisterString(address, channel->readRegister(address / 4)) << std::flush;
      } else {
        std::cout << "Done!" << std::endl;
      }
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramWriteRegister().execute(argc, argv);
}
