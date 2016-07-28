///
/// \file RorcUtilsReadRegister.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that prints the FIFO of a RORC
///
/// TODO Support FIFO file input
/// TODO Option to switch between pretty output and simple dump

#include <iostream>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include "ChannelUtilityInterface.h"
#include "RorcException.h"

using namespace AliceO2::Rorc::Util;
using namespace AliceO2::Rorc;
namespace b = boost;
using std::cout;
using std::endl;

class ProgramPrintFifo: public RorcUtilsProgram
{
  public:
    static constexpr int headerInterval = 32;

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

      auto channel = ChannelFactory().getMaster(serialNumber, channelNumber);
      auto utility = dynamic_cast<ChannelUtilityInterface*>(channel.get());

      if (utility) {
        utility->utilityPrintFifo(cout);
      } else {
        BOOST_THROW_EXCEPTION(RorcException()
            << errinfo_rorc_generic_message("Failed to print FIFO")
            << errinfo_rorc_possible_causes({"Unsupported card type"})
            << errinfo_rorc_card_type(channel->getCardType()));
      }
    }
};

int main(int argc, char** argv)
{
  return ProgramPrintFifo().execute(argc, argv);
}
