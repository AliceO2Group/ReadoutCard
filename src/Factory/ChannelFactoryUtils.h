/// \file ChannelFactoryUtils.h
/// \brief Definition of helper functions for Channel factories
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <map>
#include <algorithm>
#include "CardDescriptor.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/Parameters.h"
#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
# include "RocPciDevice.h"
#endif

namespace AliceO2 {
namespace roc {
namespace ChannelFactoryUtils {

#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
inline CardDescriptor findCard(int serial)
{
  auto cardsFound = RocPciDevice::findSystemDevices(serial);

  if (cardsFound.empty()) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Could not find a card with the given serial number")
        << ErrorInfo::SerialNumber(serial));
  }

  if (cardsFound.size() > 1) {
    std::vector<PciId> pciIds;
    for (const auto& c : cardsFound) { pciIds.push_back(c.pciId); }
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Found more than one card with the given serial number")
        << ErrorInfo::SerialNumber(serial)
        << ErrorInfo::PciIds(pciIds));
  }

  return cardsFound.at(0);
}

inline CardDescriptor findCard(const PciAddress& address)
{
  auto cardsFound = RocPciDevice::findSystemDevices(address);

  if (cardsFound.empty()) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Could not find a card with the given PCI address number")
        << ErrorInfo::PciAddress(address));
  }

  if (cardsFound.size() > 1) {
    std::vector<PciId> pciIds;
    for (const auto& c : cardsFound) { pciIds.push_back(c.pciId); }
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Found more than one card with the given PCI address number")
        << ErrorInfo::PciAddress(address)
        << ErrorInfo::PciIds(pciIds));
  }

  return cardsFound.at(0);
}

inline CardDescriptor findCard(const Parameters::CardIdType& id)
{
  if (auto serialMaybe = boost::get<int>(&id)) {
    auto serial = *serialMaybe;
    return findCard(serial);
  } else if (auto addressMaybe = boost::get<PciAddress>(&id)) {
    auto address = *addressMaybe;
    return findCard(address);
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Invalid Card ID"));
  }
}
#endif

/// Helper template method for the channel factories.
/// \param serialNumber Serial number of the card
/// \param dummySerial Serial number that indicates a dummy object should be instantiated
/// \param map A mapping of CardType to functions that create a shared pointer to an implementation of the Interface
///   Formed by pairs of arguments of a CardType and an std::function.
///   Example:
///   auto sharedPointer = channelFactoryHelper<ChannelMasterInterface>(12345, -1, {
///       { CardType::Dummy, [](){ return makeDummy(); }},
///       { CardType::Crorc, [](){ return makeCrorc(); }}});
template <typename Interface>
std::unique_ptr<Interface> channelFactoryHelper(const Parameters& params, int dummySerial,
    const std::map<CardType::type, std::function<std::unique_ptr<Interface>()>>& map)
{
  auto makeDummy = [&]() {
    auto iter = map.find(CardType::Dummy);
    if (iter != map.end()) {
      return iter->second();
    } else {
      BOOST_THROW_EXCEPTION(Exception()
          << ErrorInfo::Message("Instantiation function for Dummy card type not available"));
    }
  };

#ifndef ALICEO2_READOUTCARD_PDA_ENABLED
  // If PDA is NOT enabled we only make dummies
  return makeDummy();
#else

  auto id = params.getCardIdRequired();

  // First check if we got a dummy serial number
  if (auto serialMaybe = boost::get<int>(&id)) {
    auto serial = *serialMaybe;
    if (serial == dummySerial) {
      return makeDummy();
    }
  }

  // Else, find the card with the given ID, and execute the instantiation function corresponding to the card's type.
  auto cardDescriptor = findCard(id);

  auto iter = map.find(cardDescriptor.cardType);
  if (iter != map.end()) {
    return iter->second();
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Instantiation function for card type not available")
        << ErrorInfo::CardType(cardDescriptor.cardType) << ErrorInfo::CardId(id));
  }
#endif
}

} // namespace ChannelFactoryUtils
} // namespace roc
} // namespace AliceO2
