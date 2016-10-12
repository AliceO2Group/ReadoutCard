/// \file ProgramRegisterReadRange.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that reads a range of registers from a RORC

#include "Utilities/Program.h"
#include "RORC/ChannelFactory.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace AliceO2::Rorc::Utilities;
namespace po = boost::program_options;

namespace {
class ProgramRegisterReadRange: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return {"Read Register Range", "Read a range of registers",
          "./rorc-reg-read-range --serial=12345 --channel=0 --address=0x8 --range=10"};
    }

    virtual void addOptions(po::options_description& options)
    {
      Options::addOptionRegisterAddress(options);
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      Options::addOptionRegisterRange(options);
      options.add_options()("file", po::value<std::string>(&mFile), "Output to given file in binary format");
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int baseAddress = Options::getOptionRegisterAddress(map);
      int channelNumber = Options::getOptionChannel(map);
      int range = Options::getOptionRegisterRange(map);
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      std::vector<uint32_t> values(range);

      // Registers are indexed by 32 bits (4 bytes)
      int baseIndex = baseAddress / 4;

      for (int i = 0; i < range; ++i) {
        values[i] = channel->readRegister(baseIndex + i);
      }

      if (mFile.empty()) {
        for (int i = 0; i < range; ++i) {
          std::cout << Common::makeRegisterString((baseIndex + i) * 4, values[i]);
        }
      }
      else {
        std::ofstream stream(mFile, std::ios_base::out);
        stream.write(reinterpret_cast<char*>(values.data()), values.size() * sizeof(decltype(values)::value_type));
      }
    }

  private:

    std::string mFile;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramRegisterReadRange().execute(argc, argv);
}
