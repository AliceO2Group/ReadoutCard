/// \file CardConfigurator.cxx
/// \brief Class to interface the card's BAR for configuration.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Common/Configuration.h"
#include "Configuration/ConfigurationFactory.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/CardConfigurator.h"
#include "ReadoutCard/ChannelFactory.h"

namespace AliceO2
{
namespace roc
{

CardConfigurator::CardConfigurator(Parameters::CardIdType cardId, std::string configUri, bool forceConfigure)
{
  auto parameters = Parameters::makeParameters(cardId, 2); //have to make parameters for this case, bar2
  try {
    parseConfigUri(configUri, parameters);
  } catch (...) {
    throw;
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

/// configUri has to start with "ini://", "json://" or "consul://"
void CardConfigurator::parseConfigUri(std::string configUri, Parameters& parameters)
{
  std::set<uint32_t> linkMask;
  std::map<uint32_t, GbtMux::type> gbtMuxMap;

  Clock::type clock = Clock::type::Local;
  DatapathMode::type datapathMode = DatapathMode::type::Packet;
  bool allowRejection = false;
  bool loopback = false;
  bool ponUpstream = false;
  bool dynamicOffset = false;
  uint32_t onuAddress = 0x0;
  uint16_t cruId = 0x0;
  GbtMode::type gbtMode = GbtMode::type::Gbt;
  DownstreamData::type downstreamData = DownstreamData::type::Ctp;

  bool enabled = false;
  std::string gbtMux = "ttc";

  std::unique_ptr<o2::configuration::ConfigurationInterface> conf;
  try {
    conf = o2::configuration::ConfigurationFactory::getConfiguration(configUri);
  } catch (std::exception& e) {
    std::cout << boost::diagnostic_information(e) << std::endl;
    throw;
  }

  auto tree = conf->getRecursive("");
  std::string group = "";
  try {
    for (auto it : tree) {
      group = it.first;

      if (group == "cru") { // Configure the CRU globally

        std::string parsedString;
        conf->setPrefix(group);

        parsedString = conf->get<std::string>("clock");
        clock = Clock::fromString(parsedString);

        parsedString = conf->get<std::string>("datapathMode");
        datapathMode = DatapathMode::fromString(parsedString);

        parsedString = conf->get<std::string>("gbtMode");
        gbtMode = GbtMode::fromString(parsedString);

        parsedString = conf->get<std::string>("downstreamData");
        downstreamData = DownstreamData::fromString(parsedString);

        loopback = conf->get<bool>("loopback");
        ponUpstream = conf->get<bool>("ponUpstream");
        dynamicOffset = conf->get<bool>("dynamicOffset");

        parsedString = conf->get<std::string>("onuAddress");
        onuAddress = Hex::fromString(parsedString);

        parsedString = conf->get<std::string>("cruId");
        cruId = Hex::fromString(parsedString);

        allowRejection = conf->get<bool>("allowRejection");

        conf->setPrefix("");

        parameters.setClock(clock);
        parameters.setDatapathMode(datapathMode);
        parameters.setGbtMode(gbtMode);
        parameters.setDownstreamData(downstreamData);
        parameters.setLinkLoopbackEnabled(loopback);
        parameters.setPonUpstreamEnabled(ponUpstream);
        parameters.setDynamicOffsetEnabled(dynamicOffset);
        parameters.setOnuAddress(onuAddress);
        parameters.setCruId(cruId);
        parameters.setAllowRejection(allowRejection);

      } else if (group == "links") { // Configure all links with default values

        conf->setPrefix(group);
        enabled = conf->get<bool>("enabled");

        if (enabled) {
          for (int i = 0; i < 24; i++) {
            linkMask.insert((uint32_t)i);
          }
        }

        gbtMux = conf->get<std::string>("gbtMux");
        for (int i = 0; i < 24; i++) {
          gbtMuxMap.insert(std::make_pair((uint32_t)i, GbtMux::fromString(gbtMux)));
        }

        conf->setPrefix("");

      } else if (!group.find("link")) { // Configure individual links

        std::string linkIndexString = group.substr(group.find("k") + 1);
        uint32_t linkIndex = std::stoul(linkIndexString, NULL, 10);

        conf->setPrefix(group);

        enabled = conf->get<bool>("enabled");
        if (enabled) {
          linkMask.insert(linkIndex);
        } else {
          linkMask.erase(linkIndex);
        }

        gbtMux = conf->get<std::string>("gbtMux");
        if (gbtMuxMap.find(linkIndex) != gbtMuxMap.end()) {
          gbtMuxMap[linkIndex] = GbtMux::fromString(gbtMux);
        } else {
          gbtMuxMap.insert(std::make_pair(linkIndex, GbtMux::fromString(gbtMux)));
        }

        conf->setPrefix("");
      }
    }

    parameters.setLinkMask(linkMask);
    parameters.setGbtMuxMap(gbtMuxMap);

  } catch (...) {
    BOOST_THROW_EXCEPTION(ParseException() << ErrorInfo::ConfigParse(group));
  }
}

} // namespace roc
} // namespace AliceO2
