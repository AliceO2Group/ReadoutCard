/// \file ProgramMetrics.cxx
/// \brief Tool that returns current information about RoCs.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "Cru/Constants.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "RocPciDevice.h"
#include <boost/format.hpp>
#include <boost/optional/optional_io.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramMetrics: public Program
{
  public:

  virtual Description getDescription()
  {
    return {"Metrics", "Return current RoC parameters", 
      "roc-metrics\n"
      "roc-metrics --pci-address 42:00.0\n"};
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()
      ("pci-address",
       po::value<std::string>(&mOptions.pciAddress)->default_value("-1"),
       "Card's PCI Address");
  }

  virtual void run(const boost::program_options::variables_map& map) 
  {

    std::vector<CardDescriptor> cardsFound;
    if (mOptions.pciAddress != "-1")
      cardsFound = RocPciDevice::findSystemDevices(mOptions.pciAddress);
    else
      cardsFound = RocPciDevice::findSystemDevices();
      

    std::ostringstream table;

    auto formatHeader = "  %-3s %-6s %-10s %-10s %-19s %-20s %-19s %-8s %-17s %-17s\n";
    auto formatRow = "  %-3s %-6s %-10s %-10s %-19s %-20s %-19s %-8s %-17s %-17s\n";
    auto header = (boost::format(formatHeader)
        % "#" % "Type" % "PCI Addr" % "Temp (C)" % "#Dropped Packets" % "CTP Clock (MHz)" % "Local Clock (MHz)"
        % "#links" % "#Wrapper 0 links" % "#Wrapper 1 links").str();
    auto lineFat = std::string(header.length(), '=') + '\n';
    auto lineThin = std::string(header.length(), '-') + '\n';

    table << lineFat << header << lineThin;

    int i = 0;
    for (const auto& card : cardsFound) {
      Parameters params = Parameters::makeParameters(card.pciAddress, 2);
      auto bar2 = ChannelFactory().getBar(params);

      float temperature = bar2->getTemperature().value_or(0);
      int32_t dropped = bar2->getDroppedPackets();
      float ctp_clock = bar2->getCTPClock()/1e6;
      float local_clock = bar2->getLocalClock()/1e6;
      int32_t links = bar2->getLinks();
      uint32_t links0 = bar2->getLinksPerWrapper(0);
      uint32_t links1 = bar2->getLinksPerWrapper(1);

      auto format = boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciAddress.toString()
        % temperature % dropped % ctp_clock % local_clock % links % links0 % links1;

      table << format;
      i++;
    }

    table << lineFat;
    std::cout << table.str();
  }
  
  struct OptionsStruct 
  {
    std::string pciAddress = "-1";
  }mOptions;

  private:
};

int main(int argc, char** argv)
{
  if (argc > 2){
    std::cout << "Too many arguments" << std::endl;
    return -1;
  }
  return ProgramMetrics().execute(argc, argv);
}
