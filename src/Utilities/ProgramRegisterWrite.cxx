///
/// \file RorcUtilsWriteRegister.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that writes to a register on a RORC
///

#include "Utilities/Program.h"
#include "RORC/ChannelFactory.h"

using namespace AliceO2::Rorc::Utilities;

namespace {

const char* NOREAD_SWITCH("noread");

class ProgramRegisterWrite: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Write Register", "Write a value to a single register",
          "./rorc-reg-write --serial=12345 --channel=0 --address=0x8 --value=0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      Options::addOptionRegisterValue(options);
      options.add_options()(NOREAD_SWITCH, "No readback of register after write");
    }

    virtual void run(const boost::program_options::variables_map& map)
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
  return ProgramRegisterWrite().execute(argc, argv);
}
