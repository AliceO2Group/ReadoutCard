/// \file ProgramConfig.cxx
/// \brief Tool that configures the CRU.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "ReadoutCard/ChannelFactory.h"
#include "CardConfigurator.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Cru/CruBar.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramConfig: public Program
{
  public:

  virtual Description getDescription()
  {
    return {"Config", "Configure the CRU(s)", 
      "roc-config -cfg roc.cfg\n"
      "roc-config -pci-address 42:00.0 -link 0 -clock local -mode packet -loopback -mux ttc\n"};
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()
      ("clock",
       po::value<std::string>(&mOptions.clock)->default_value("LOCAL"),
       "Clock [LOCAL, TTC]")
      ("datapathmode",
       po::value<std::string>(&mOptions.datapathMode)->default_value("PACKET"),
       "DatapathMode [PACKET, CONTINUOUS]")
      ("downstreamdata",
       po::value<std::string>(&mOptions.downstreamData)->default_value("CTP"),
       "DownstreamData [CTP, PATTERN, MIDTRG]")
      ("gbtmode",
       po::value<std::string>(&mOptions.gbtMode)->default_value("GBT"),
       "GBT MODE [GBT, WB]")
      ("gbtmux",
       po::value<std::string>(&mOptions.gbtMux)->default_value("TTC"),
       "GBT MUX [TTC, DDG, SC]")
      ("links", 
       po::value<std::string>(&mOptions.links)->default_value("0"),
       "Links to enable")
      ("loopback",
       po::value<std::string>(&mOptions.loopbackMode)->default_value("NONE"),
       "Loopback mode [NONE, INTERNAL]")
      ("config-file",
       po::value<std::string>(&mOptions.configFile)->default_value(""),
       "Configuration file [file:*.cfg]");
    Options::addOptionCardId(options);
  }

  virtual void run(const boost::program_options::variables_map& map) 
  {
    auto cardId = Options::getOptionCardId(map);

    if (mOptions.configFile == "") {
      std::cout << "Configuring with command line arguments" << std::endl;
      auto params = Parameters::makeParameters(cardId, 2);
      params.setLinkMask(Parameters::linkMaskFromString(mOptions.links));
      params.setClock(Clock::fromString(mOptions.clock));
      params.setDatapathMode(DatapathMode::fromString(mOptions.datapathMode));
      params.setDownstreamData(DownstreamData::fromString(mOptions.downstreamData));
      params.setGbtMode(GbtMode::fromString(mOptions.gbtMode));
      params.setGbtMux(GbtMux::fromString(mOptions.gbtMux));
      params.setGeneratorLoopback(LoopbackMode::fromString(mOptions.loopbackMode));

      if ((params.getGeneratorLoopbackRequired() != LoopbackMode::type::Internal) && 
          (params.getGeneratorLoopbackRequired() != LoopbackMode::type::None)) {
        std::cout << "Unsupported loopback mode; Defaulting to NONE" << std::endl;
        params.setGeneratorLoopback(LoopbackMode::type::None);
      }

      auto cardConfigurator = CardConfigurator(params);
    } else if (!strncmp(mOptions.configFile.c_str(), "file:", 5)) {
      std::cout << "Configuring with config file" << std::endl;
      try {
        auto cardConfigurator = CardConfigurator(cardId, mOptions.configFile);
      } catch(...) {
        std::cout << "Something went badly reading the configuration file..." << std::endl;
      }
    } else {
      std::cout << "Configuration file path should start with 'file:'" << std::endl;
    }
  }
  
  struct OptionsStruct 
  {
    std::string clock = "local";
    std::string datapathMode = "packet";
    std::string downstreamData = "Ctp";
    std::string gbtMode = "gbt";
    std::string gbtMux = "ttc";
    std::string links = "0";
    std::string loopbackMode = "NONE";
    std::string configFile = "";
  }mOptions;

  private:
};

int main(int argc, char** argv)
{
  return ProgramConfig().execute(argc, argv);
}
