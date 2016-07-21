///
/// \file RorcUtilsWriteRegister.cxx
/// \author Pascal Boeschoten
///
/// Utility that writes to a register on a RORC
///

#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"

using namespace AliceO2::Rorc::Util;

class ProgramWriteRegister: public RorcUtilsProgram
{
  public:
    virtual ~ProgramWriteRegister()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Write Register", "Write a value to a single register", "./rorc-reg-write -a0x8 -v0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      options.add_options()("value,v", boost::program_options::value<uint32_t>()->required(), "Register value");
    }

    virtual void mainFunction(boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int address = Options::getOptionRegisterAddress(map);
      int channelNumber = Options::getOptionChannel(map);
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      // Registers are indexed by 32 bits (4 bytes)
      channel->writeRegister(address / 4, registerValue);
    }

    uint32_t registerValue;
};

int main(int argc, char** argv)
{
  return ProgramWriteRegister().execute(argc, argv);
}
