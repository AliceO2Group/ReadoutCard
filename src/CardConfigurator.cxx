/// \file CardConfigurator.cxx
/// \brief Class to interface the card's BAR for configuration.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "CardConfigurator.h"
#include "Common/Configuration.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/ChannelFactory.h"
#include <iostream>

namespace AliceO2 {
namespace roc {

CardConfigurator::CardConfigurator(Parameters::CardIdType cardId, std::string pathToConfigFile, bool forceConfigure)
{ 
  auto parameters = Parameters::makeParameters(cardId, 2); //have to make parameters for this case, bar2
  parseConfigFile(pathToConfigFile, parameters);

  auto bar2 = ChannelFactory().getBar(parameters);
  if (forceConfigure) {
    bar2->configure();
  } else {
    bar2->reconfigure();
  }
}

CardConfigurator::CardConfigurator(Parameters& parameters, bool forceConfigure)
{
  auto bar2 = ChannelFactory().getBar(parameters);
  if (forceConfigure) {
    bar2->configure();
  } else {
    bar2->reconfigure();
  }
}


/// pathToConfigFile: Has to start with "file:"
void CardConfigurator::parseConfigFile(std::string pathToConfigFile, Parameters& parameters)
{
  ConfigFile configFile;
  std::set<uint32_t> linkMask;
  std::map<uint32_t, GbtMux::type> gbtMuxMap;

  Clock::type clock = Clock::type::Local;
  DatapathMode::type datapathMode = DatapathMode::type::Packet;
  bool loopback = false;
  GbtMode::type gbtMode = GbtMode::type::Gbt;
  DownstreamData::type downstreamData = DownstreamData::type::Ctp;


  //Open the file
  try {
    if (!strncmp(pathToConfigFile.c_str(), "file:", 5)) {
      configFile.load(pathToConfigFile);
      //configFile.print(); //for debugging
    } else {
      std::cout << "Configuration file or path invalid;" << std::endl;
      throw;
    }
  } catch (std::exception &e) {
    throw std::string(e.what());
  }

  //* Global *//
  for (auto globalGroup: ConfigFileBrowser(&configFile, "cru")) { //Is there another way to do this?
    std::string parsedString;
    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".clock");
      clock = Clock::fromString(parsedString);
    } catch(...) {
      std::cout << "Invalid or missing clock property" << std::endl;
    }

    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".datapathmode");
      datapathMode = DatapathMode::fromString(parsedString);
    } catch(...) {
      std::cout << "Invalid or missing datapath mode property" << std::endl;
    }

    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".gbtmode");
      gbtMode = GbtMode::fromString(parsedString);
    } catch(...) {
      std::cout << "Invalid or missing gbtmode property" << std::endl;
    }

    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".downstreamdata");
      downstreamData = DownstreamData::fromString(parsedString);
    } catch(...) {
      std::cout << "Invalid or missing downstreamdata property" << std::endl;
    }

    try {
      loopback = configFile.getValue<bool>(globalGroup + ".loopback");
    } catch(...) {
      std::cout << "Invalid or missing loopback property" << std::endl;
    }
  }

  parameters.setLinkLoopbackEnabled(loopback);
  parameters.setClock(clock);
  parameters.setDatapathMode(datapathMode);
  parameters.setGbtMode(gbtMode);
  parameters.setDownstreamData(downstreamData);

  //* Per link *//
  for (auto configGroup: ConfigFileBrowser(&configFile, "link")) {

    bool enabled = false;
    std::string gbtMux;

    /* configure for all links */
    if (configGroup == "links") {
      try {
        enabled = configFile.getValue<bool>(configGroup + ".enabled");
        if (enabled) {
          for (int i=0; i<24; i++) {
            linkMask.insert((uint32_t) i);
          }
        }
      } catch (...) {
        std::cout << "Invalid or missing enabled property for all links" << std::endl;
      }

      try {
        gbtMux = configFile.getValue<std::string>(configGroup + ".gbtmux");
        for (int i=0; i<24; i++) {
          gbtMuxMap.insert(std::make_pair((uint32_t) i, GbtMux::fromString(gbtMux)));
        }
      } catch(...) {
        std::cout << "Invalid or missing gbt mux property for all links" << std::endl;
      }
      continue;
    }

    /* configure for individual links */
    std::string linkIndexString = configGroup.substr(configGroup.find("k") + 1);
    uint32_t linkIndex = std::stoul(linkIndexString, NULL, 10); 

    try {
      enabled = configFile.getValue<bool>(configGroup + ".enabled");
      if (enabled) {
        linkMask.insert(linkIndex);
      } else {
        linkMask.erase(linkIndex);
      }
    } catch(...) {
      std::cout << "Invalid or missing enabled property for link: " << linkIndexString << std::endl;
    }

    try {
      gbtMux = configFile.getValue<std::string>(configGroup + ".gbtmux");
      if (gbtMuxMap.find(linkIndex) != gbtMuxMap.end()) {
        gbtMuxMap[linkIndex] = GbtMux::fromString(gbtMux);
      } else {
        gbtMuxMap.insert(std::make_pair(linkIndex, GbtMux::fromString(gbtMux)));
      }
    } catch(...) {
      std::cout << "Invalid or missing gbt mux set for link: " << linkIndexString << std::endl;
    }
  }

  parameters.setLinkMask(linkMask);
  parameters.setGbtMuxMap(gbtMuxMap);
}
} // namespace roc
} // namespace AliceO2
