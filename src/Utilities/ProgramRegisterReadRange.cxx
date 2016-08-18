///
/// \file RorcUtilsReadRegisterRange.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that reads a range of registers from a RORC
///

#include "Utilities/Program.h"
#include "RORC/ChannelFactory.h"
#include <iostream>

using namespace AliceO2::Rorc::Utilities;

namespace {
class ProgramRegisterReadRange: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Read Register Range", "Read a range of registers",
          "./rorc-reg-read-range --serial=12345 --channel=0 -a0x8 -r10");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      Options::addOptionRegisterRange(options);
    }

    virtual void run(const boost::program_options::variables_map& map)
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
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramRegisterReadRange().execute(argc, argv);
}
