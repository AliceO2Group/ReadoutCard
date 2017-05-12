/// \file ProgramFlashRead.cxx
/// \brief Utility that reads the flash
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iostream>
#include <string>
#include "ReadoutCard/ChannelFactory.h"
#include "Crorc/Crorc.h"
#include "ExceptionInternal.h"

using namespace AliceO2::roc::CommandLineUtilities;
using std::cout;
using std::endl;
namespace po = boost::program_options;

namespace {
class ProgramCrorcFlash: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"Flash Read", "Reads card flash memory",
        "roc-flash-read --id=12345 --address=0 --words=32"};
    }

    virtual void addOptions(po::options_description& options)
    {
      Options::addOptionCardId(options);
      options.add_options()
          ("address", po::value<uint64_t>(&mAddress)->default_value(0), "Starting address to read")
          ("words", po::value<uint64_t>(&mWords)->required(), "Amount of 32-bit words to read");
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      using namespace AliceO2::roc;

      auto cardId = Options::getOptionCardId(map);
      auto channelNumber = 0;
      auto params = AliceO2::roc::Parameters::makeParameters(cardId, channelNumber);
      auto channel = AliceO2::roc::ChannelFactory().getBar(params);

      if (channel->getCardType() != CardType::Crorc) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Only C-RORC supported for now"));
      }

      Crorc::readFlashRange(*channel.get(), mAddress, mWords, std::cout);
    }

    uint64_t mAddress = 0;
    uint64_t mWords = 0;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCrorcFlash().execute(argc, argv);
}
