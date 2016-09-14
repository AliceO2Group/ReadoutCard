///
/// \file ProgramInitChannel.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that initializes a RORC channel
///

#include <Utilities/Common.h>
#include <Utilities/Options.h>
#include <Utilities/Program.h>
#include <iostream>
#include "RORC/ChannelFactory.h"
#include "RorcDevice.h"
#include <Configuration/ConfigurationFactory.h>
#include <boost/format.hpp>

using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;
namespace b = boost;

namespace {
  const std::string CONF_URI_OPTION = "conf-uri";
}

namespace {
class ProgramInitChannel: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("Initialize Channel", "Initializes a RORC channel",
          "./rorc-init-channel --serial=12345 --channel=0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      namespace po = boost::program_options;
      Options::addOptionSerialNumber(options);
      Options::addOptionChannel(options);
      Options::addOptionsChannelParameters(options);
      options.add_options()
          (CONF_URI_OPTION.c_str(), po::value<std::string>(), "Use Configuration URI to get channel parameters");
    }

    ChannelParameters getParametersFromConfiguration(const std::string& uri, CardType::type cardType, int serial,
        int channel)
    {
      auto conf = ConfigurationFactory::getConfiguration(uri);
      const auto prefix = b::str(b::format("/RORC/card_%s/serial_%i/channel_%i/") % CardType::toString(cardType)
          % serial % channel);

      std::map<std::string, std::string> map;
      auto get = [&](const std::string& param) {
        std::string key = prefix + param;
        map.emplace(key, conf->getString(key));
      };

      get("dma_page_size");
      get("dma_buffer_size");
      get("generator_enabled");
      get("generator_loopback_mode");

      ChannelParameters cp;

      cp.dma.bufferSize;

      return cp;
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      auto serialNumber = Options::getOptionSerialNumber(map);
      auto channelNumber = Options::getOptionChannel(map);

      auto parameters = Options::getOptionsChannelParameters(map);

      auto cardsFound = RorcDevice::findSystemDevices();

      for (auto& card : cardsFound) {
        if (serialNumber == card.serialNumber) {
          cout << "Found card, initializing channel..." << endl;

          if (card.cardType == CardType::Cru) {
            if (parameters.dma.pageSize != 8l*1024l) {
              cout << "Warning: given page size != 8 kiB, required for CRU. Correcting automatically.\n";
              parameters.dma.pageSize = 8l*1024l;
            }
          }

          auto parameters = map.count(CONF_URI_OPTION)
              ? getParametersFromConfiguration(
                  map.at(CONF_URI_OPTION).as<std::string>(), card.cardType, serialNumber, channelNumber)
              : Options::getOptionsChannelParameters(map);

          // Don't assign it to any value so it constructs and destructs completely before we say things are done
          ChannelFactory().getMaster(serialNumber, channelNumber, parameters);
          cout << "Done!" << endl;
          return;
        }
      }

      cout << "Could not find card with given serial number\n";
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramInitChannel().execute(argc, argv);
}
