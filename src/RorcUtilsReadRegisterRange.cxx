///
/// \file RorcUtilsReadRegisterRange.cxx
/// \author Pascal Boeschoten
///
/// Utility that reads a range of registers from a RORC
///

#include <iostream>
#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"

using namespace AliceO2::Rorc::Util;

class ProgramReadRegisterRange: public RorcUtilsProgram
{
  public:
    virtual ~ProgramReadRegisterRange()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Read Register Range", "Read a range of registers", "./rorc-reg-read-range -a0x8 -r10");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      Options::addOptionRegisterRange(options);
    }

    virtual void mainFunction(boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int baseAddress = Options::getOptionRegisterAddress(map);
      int channelNumber = Options::getOptionChannel(map);
      int range = Options::getOptionRegisterRange(map);
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      // Registers are indexed by 32 bits (4 bytes)
      int baseIndex = baseAddress / 4;

      for (int i = 0; i < range; ++i) {
        uint32_t value = channel->readRegister(baseIndex + i);
        std::cout << Common::makeRegisterString((baseIndex + i) * 4, value);
      }
    }
};

int main(int argc, char** argv)
{
  return ProgramReadRegisterRange().execute(argc, argv);
}
