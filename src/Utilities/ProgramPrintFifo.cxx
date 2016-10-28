/// \file ProgramPrintFifo.cxx
/// \brief Utility that prints the FIFO of a RORC
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// TODO Support FIFO file input

#include <iostream>
#include <iomanip>
#include <boost/exception/diagnostic_information.hpp>
#include "Utilities/Program.h"
#include "Factory/ChannelUtilityFactory.h"
#include "RORC/Exception.h"

using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
namespace b = boost;
using std::cout;
using std::endl;

namespace {
class ProgramPrintFifo: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Print FIFO", "Prints the FIFO of a RORC",
          "./rorc-print-fifo --serial=12345 --channel=0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      options.add_options()("nopretty", "Dump FIFO contents instead of making a nice table");
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int channelNumber = Options::getOptionChannel(map);
      auto params = AliceO2::Rorc::Parameters::makeParameters(serialNumber, channelNumber);
      auto channel = ChannelUtilityFactory().getUtility(params);

      if (map.count("nopretty")) {
        // Dump contents
        auto fifo = channel->utilityCopyFifo();
        for (size_t i = 0; i < fifo.size(); ++i) {
          auto value = fifo[i];
          cout << std::setw(4) << i << "  =>  0x" << Common::make32hexString(value)
              << "  =  0b" << Common::make32bitString(value) << "  =  " << value << '\n';
        }
      }
      else {
        // Nice printed FIFO
        channel->utilityPrintFifo(cout);
      }
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramPrintFifo().execute(argc, argv);
}
