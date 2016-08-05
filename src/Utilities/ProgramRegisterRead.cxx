///
/// \file ProgramRegisterRead.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that reads a register from a RORC
///

#include "Utilities/Program.h"
#include <iostream>
#include "RORC/ChannelFactory.h"
#include "RORC/ChannelParameters.h"

namespace {
using namespace AliceO2::Rorc::Utilities;

class ProgramRegisterRead: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Read Register", "Read a single register",
          "./rorc-reg-read --serial=12345 --channel=0 --address=0x8");
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
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramRegisterRead().execute(argc, argv);
}
