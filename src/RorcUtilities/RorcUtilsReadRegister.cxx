///
/// \file RorcUtilsReadRegister.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that reads a register from a RORC
///

#include <iostream>
#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include "RORC/ChannelParameters.h"

using namespace AliceO2::Rorc::Util;

class ProgramReadRegister: public RorcUtilsProgram
{
  public:
    virtual ~ProgramReadRegister()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Read Register", "Read a single register", "./rorc-reg-read --serial=12345 --address=0x8");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int address = Options::getOptionRegisterAddress(map);
      int channelNumber = Options::getOptionChannel(map);
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      // Registers are indexed by 32 bits (4 bytes)
      uint32_t value = channel->readRegister(address / 4);
      std::cout << Common::makeRegisterString(address, value);
    }
};

int main(int argc, char** argv)
{
  return ProgramReadRegister().execute(argc, argv);
}
