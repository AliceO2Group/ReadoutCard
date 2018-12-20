/// \file ProgramRegisterModify.cxx
/// \brief Utility that modifies bits of a register on a card
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "CommandLineUtilities/Program.h"
#include "ReadoutCard/ChannelFactory.h"

using namespace AliceO2::roc::CommandLineUtilities;
namespace po = boost::program_options;

const char* NOREAD_SWITCH("noread");

class ProgramRegisterModify: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"Modify Register", "Modify bits of a single register",
          "roc-reg-modify --id=12345 --channel=0 --address=0x8 --position=3 --width=1 --value=0"};
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionCardId(options);
      Options::addOptionRegisterValue(options);
      options.add_options()
        ("position",
         po::value<int>(&mOptions.position)->default_value(0),
         "Position to modify bits on")
        ("width",
         po::value<int>(&mOptions.width)->default_value(1),
         "Number of bits to modify")
        (NOREAD_SWITCH, 
         "No readback of register before and after write");
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      auto cardId = Options::getOptionCardId(map);
      int address = Options::getOptionRegisterAddress(map);
      int channelNumber = Options::getOptionChannel(map);
      int registerValue = Options::getOptionRegisterValue(map);
      int position = mOptions.position;
      int width = mOptions.width;
      auto readback = !bool(map.count(NOREAD_SWITCH));
      auto params = AliceO2::roc::Parameters::makeParameters(cardId, channelNumber);
      auto channel = AliceO2::roc::ChannelFactory().getBar(params);

      // Registers are indexed by 32 bits (4 bytes)
      if (readback) {
        auto value = channel->readRegister(address/4);
        if (isVerbose()) {
          std::cout << "Before modification:" << std::endl;
          std::cout << Common::makeRegisterString(address, value) << '\n';
        } else {
          std::cout << "0x" << std::hex << value << '\n';
        }
      }

      channel->modifyRegister(address/4, position, width, registerValue);

      if (readback) {
        auto value = channel->readRegister(address/4);
        if (isVerbose()) {
          std::cout << "After modification:" << std::endl;
          std::cout << Common::makeRegisterString(address, value) << '\n';
        } else {
          std::cout << "0x" << std::hex << value << '\n';
        }
      } else {
        std::cout << (isVerbose() ? "Done!\n" : "\n");
      }
    }

    struct OptionsStruct
    {
      int position = 0;
      int width = 1;
    }mOptions;
};

int main(int argc, char** argv)
{
  return ProgramRegisterModify().execute(argc, argv);
}
