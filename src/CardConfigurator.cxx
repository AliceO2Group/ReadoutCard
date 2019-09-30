/// \file CardConfigurator.cxx
/// \brief Class to interface the card's BAR for configuration.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Common/Configuration.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/CardConfigurator.h"
#include "ReadoutCard/ChannelFactory.h"

namespace AliceO2
{
namespace roc
{

CardConfigurator::CardConfigurator(Parameters::CardIdType cardId, std::string pathToConfigFile, bool forceConfigure)
{
  auto parameters = Parameters::makeParameters(cardId, 2); //have to make parameters for this case, bar2
  try {
    parseConfigFile(pathToConfigFile, parameters);
  } catch (std::string e) {
    throw std::runtime_error(e);
  }

  auto bar2 = ChannelFactory().getBar(parameters);
  try {
    if (forceConfigure) {
      bar2->configure();
    } else {
      bar2->reconfigure();
    }
  } catch (const Exception& e) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(boost::diagnostic_information(e)));
  }
}

CardConfigurator::CardConfigurator(Parameters& parameters, bool forceConfigure)
{
  auto bar2 = ChannelFactory().getBar(parameters);
  try {
    if (forceConfigure) {
      bar2->configure();
    } else {
      bar2->reconfigure();
    }
  } catch (const Exception& e) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(boost::diagnostic_information(e)));
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
  bool allowRejection = false;
  bool loopback = false;
  bool ponUpstream = false;
  uint32_t onuAddress = 0x0;
  uint16_t cruId = 0x0;
  GbtMode::type gbtMode = GbtMode::type::Gbt;
  DownstreamData::type downstreamData = DownstreamData::type::Ctp;

  //Open the file
  try {
    if (!strncmp(pathToConfigFile.c_str(), "file:", 5)) {
      configFile.load(pathToConfigFile);
      //configFile.print(); //for debugging
    } else {
      throw std::runtime_error("Configuration or path invalid");
    }
  } catch (std::string e) {
    throw std::runtime_error(e);
  }

  //* Global *//
  for (auto globalGroup : ConfigFileBrowser(&configFile, "cru")) { //Is there another way to do this?
    std::string parsedString;
    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".clock");
      clock = Clock::fromString(parsedString);
    } catch (...) {
      throw std::runtime_error("Invalid or missing clock property");
    }

    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".datapathmode");
      datapathMode = DatapathMode::fromString(parsedString);
    } catch (...) {
      throw std::runtime_error("Invalid or missing datapath mode property");
    }

    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".gbtmode");
      gbtMode = GbtMode::fromString(parsedString);
    } catch (...) {
      throw("Invalid or missing gbtmode property");
    }

    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".downstreamdata");
      downstreamData = DownstreamData::fromString(parsedString);
    } catch (...) {
      throw("Invalid or missing downstreamdata property");
    }

    try {
      loopback = configFile.getValue<bool>(globalGroup + ".loopback");
    } catch (...) {
      throw("Invalid or missing loopback property");
    }

    try {
      ponUpstream = configFile.getValue<bool>(globalGroup + ".ponupstream");
    } catch (...) {
      throw("Invalid or missing ponupstream property");
    }

    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".onuaddress");
      onuAddress = Hex::fromString(parsedString);
    } catch (...) {
      throw("Invalid or missing onuAddress property");
    }

    try {
      parsedString = configFile.getValue<std::string>(globalGroup + ".cruid");
      cruId = Hex::fromString(parsedString);
    } catch (...) {
      throw("Invalid or missing cruId property");
    }

    try {
      allowRejection = configFile.getValue<bool>(globalGroup + ".allowrejection");
    } catch (...) {
      throw("Invalid or missing allowrejection property");
    }
  }

  parameters.setClock(clock);
  parameters.setDatapathMode(datapathMode);
  parameters.setGbtMode(gbtMode);
  parameters.setDownstreamData(downstreamData);
  parameters.setLinkLoopbackEnabled(loopback);
  parameters.setPonUpstreamEnabled(ponUpstream);
  parameters.setOnuAddress(onuAddress);
  parameters.setCruId(cruId);
  parameters.setAllowRejection(allowRejection);

  //* Per link *//
  for (auto configGroup : ConfigFileBrowser(&configFile, "link")) {

    bool enabled = false;
    std::string gbtMux;

    /* configure for all links */
    if (configGroup == "links") {
      try {
        enabled = configFile.getValue<bool>(configGroup + ".enabled");
        if (enabled) {
          for (int i = 0; i < 24; i++) {
            linkMask.insert((uint32_t)i);
          }
        }
      } catch (...) {
        throw("Invalid or missing enabled property for all links");
      }

      try {
        gbtMux = configFile.getValue<std::string>(configGroup + ".gbtmux");
        for (int i = 0; i < 24; i++) {
          gbtMuxMap.insert(std::make_pair((uint32_t)i, GbtMux::fromString(gbtMux)));
        }
      } catch (...) {
        throw("Invalid or missing gbt mux property for all links");
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
    } catch (...) {
      throw("Invalid or missing enabled property for link: " + linkIndexString);
    }

    try {
      gbtMux = configFile.getValue<std::string>(configGroup + ".gbtmux");
      if (gbtMuxMap.find(linkIndex) != gbtMuxMap.end()) {
        gbtMuxMap[linkIndex] = GbtMux::fromString(gbtMux);
      } else {
        gbtMuxMap.insert(std::make_pair(linkIndex, GbtMux::fromString(gbtMux)));
      }
    } catch (...) {
      throw("Invalid or missing gbt mux set for link: " + linkIndexString);
    }
  }

  parameters.setLinkMask(linkMask);
  parameters.setGbtMuxMap(gbtMuxMap);
}
} // namespace roc
} // namespace AliceO2
