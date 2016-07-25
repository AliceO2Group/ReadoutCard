///
/// \file RorcUtilsReadRegister.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that prints the FIFO of a RORC
///

#include <iostream>
#include <stdexcept>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include "ChannelPaths.h"
#include "TypedMemoryMappedFile.h"
#include "ReadyFifo.h"
#include <boost/format.hpp>

using namespace AliceO2::Rorc::Util;
using std::cout;
using std::endl;

class ProgramPrintFifo: public RorcUtilsProgram
{
  public:
    virtual ~ProgramPrintFifo()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Print FIFO", "Prints the FIFO of a RORC", "./rorc-print-fifo --serial=12345 --channel=0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
    }

    virtual void mainFunction(boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int channelNumber = Options::getOptionChannel(map);
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      using namespace AliceO2::Rorc;
      auto fileName = ChannelPaths::fifo(serialNumber, channelNumber);

      cout << "Printing FIFO at '" << fileName << "'" << endl;
      auto header = boost::str(boost::format(" %-3s %-14s %-14s %-14s %-14s\n") % "#" % "Length (hex)" % "Status (hex)"
          % "Length (dec)" % "Status (dec)");
      auto lineFat = std::string(header.length(), '=') + '\n';
      auto lineThin = std::string(header.length(), '-') + '\n';

      TypedMemoryMappedFile<ReadyFifo> mappedFileFifo(fileName.c_str());
      auto fifo = mappedFileFifo.get();

      cout << lineFat << header << lineThin;

      for (size_t i = 0 ; i < fifo->entries.size(); ++i) {
        int32_t length = fifo->entries[i].length;
        int32_t status = fifo->entries[i].status;
        cout << boost::str(boost::format(" %-3d %14x %14x %14d %14d\n") % i % length % status % length % status);
        //cout << " " << i << " -> " << fifo->entries[i].length << " " << fifo->entries[i].status << endl;
      }

      cout << lineFat;
    }
};

int main(int argc, char** argv)
{
  return ProgramPrintFifo().execute(argc, argv);
}
