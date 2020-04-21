/// \file CardConfigurator.cxx
/// \brief Class to interface the card's BAR for configuration.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Common/Configuration.h"
#include "Configuration/ConfigurationFactory.h"
#include "Cru/CruBar.h"
#include "ReadoutCard/CardConfigurator.h"
#include "ReadoutCard/ChannelFactory.h"
#include "RocPciDevice.h"

namespace AliceO2
{
namespace roc
{

CardConfigurator::CardConfigurator(Parameters::CardIdType cardId, std::string configUri, bool forceConfigure) //TODO: Currently only supports the CRU
{
  auto cardType = RocPciDevice(cardId).getCardDescriptor().cardType;
  Parameters parameters;
  if (cardType == CardType::Cru) {
    parameters = Parameters::makeParameters(cardId, 2); //have to make parameters for this case, bar2
  } else if (cardType == CardType::Crorc) {
    parameters = Parameters::makeParameters(cardId, 0); //have to make parameters for this case, bar0
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Unknown card type"));
  }
  try {
    parseConfigUri(cardType, configUri, parameters);
  } catch (...) {
    throw;
  }

  auto bar2 = ChannelFactory().getBar(parameters);
  try {
    bar2->configure(forceConfigure);
  } catch (const Exception& e) {
    BOOST_THROW_EXCEPTION(e);
  }
}

CardConfigurator::CardConfigurator(Parameters& parameters, bool forceConfigure)
{
  auto cardType = RocPciDevice(parameters.getCardId().get()).getCardDescriptor().cardType;
  try {
    if (cardType == CardType::Cru) {
      auto bar2 = ChannelFactory().getBar(parameters);
      bar2->configure(forceConfigure);
    } else if (cardType == CardType::Crorc) {
      parameters.setChannelNumber(0); // we need to change bar 0
      auto bar0 = ChannelFactory().getBar(parameters);
      bar0->configure(forceConfigure);
    }
  } catch (const Exception& e) {
    BOOST_THROW_EXCEPTION(e);
  }
}

void CardConfigurator::parseConfigUri(CardType::type cardType, std::string configUri, Parameters& parameters)
{
  if (cardType == CardType::Cru) {
    parseConfigUriCru(configUri, parameters);
  } else if (cardType == CardType::Crorc) {
    parseConfigUriCrorc(configUri, parameters);
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Unknown card type"));
  }
}

void CardConfigurator::parseConfigUriCrorc(std::string /*configUri*/, Parameters& /*parameters*/) //TODO: Fill me
{
  std::cout << "Non-parameter configuration not supported for the CRORC yet" << std::endl;
  BOOST_THROW_EXCEPTION(Exception());
}
/// configUri has to start with "ini://", "json://" or "consul://"
void CardConfigurator::parseConfigUriCru(std::string configUri, Parameters& parameters)
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
  uint32_t triggerWindowSize = 1000;
  bool gbtEnabled = false;
  bool userLogicEnabled = false;

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
      auto subtree = it.second;

      if (group == "cru") { // Configure the CRU globally

        std::string parsedString;

        parsedString = subtree.get<std::string>("clock");
        clock = Clock::fromString(parsedString);

        parsedString = subtree.get<std::string>("datapathMode");
        datapathMode = DatapathMode::fromString(parsedString);

        parsedString = subtree.get<std::string>("gbtMode");
        gbtMode = GbtMode::fromString(parsedString);

        parsedString = subtree.get<std::string>("downstreamData");
        downstreamData = DownstreamData::fromString(parsedString);

        loopback = subtree.get<bool>("loopback");
        ponUpstream = subtree.get<bool>("ponUpstream");
        dynamicOffset = subtree.get<bool>("dynamicOffset");

        parsedString = subtree.get<std::string>("onuAddress");
        onuAddress = Hex::fromString(parsedString);

        parsedString = subtree.get<std::string>("cruId");
        cruId = Hex::fromString(parsedString);

        allowRejection = subtree.get<bool>("allowRejection");

        triggerWindowSize = subtree.get<int>("triggerWindowSize");

        gbtEnabled = subtree.get<bool>("gbtEnabled");
        userLogicEnabled = subtree.get<bool>("userLogicEnabled");

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
        parameters.setTriggerWindowSize(triggerWindowSize);
        parameters.setGbtEnabled(gbtEnabled);
        parameters.setUserLogicEnabled(userLogicEnabled);

      } else if (group == "links") { // Configure all links with default values

        enabled = subtree.get<bool>("enabled");

        if (enabled) {
          for (int i = 0; i < 12; i++) {
            linkMask.insert((uint32_t)i);
          }
        }

        gbtMux = subtree.get<std::string>("gbtMux");
        for (int i = 0; i < 12; i++) {
          gbtMuxMap.insert(std::make_pair((uint32_t)i, GbtMux::fromString(gbtMux)));
        }

      } else if (!group.find("link")) { // Configure individual links

        std::string linkIndexString = group.substr(group.find("k") + 1);
        uint32_t linkIndex = std::stoul(linkIndexString, NULL, 10);

        if (linkIndex > 11) {
          BOOST_THROW_EXCEPTION(ParseException() << ErrorInfo::ConfigParse(group));
        }

        enabled = subtree.get<bool>("enabled");
        if (enabled) {
          linkMask.insert(linkIndex);
        } else {
          linkMask.erase(linkIndex);
        }

        gbtMux = subtree.get<std::string>("gbtMux");
        if (gbtMuxMap.find(linkIndex) != gbtMuxMap.end()) {
          gbtMuxMap[linkIndex] = GbtMux::fromString(gbtMux);
        } else {
          gbtMuxMap.insert(std::make_pair(linkIndex, GbtMux::fromString(gbtMux)));
        }
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
