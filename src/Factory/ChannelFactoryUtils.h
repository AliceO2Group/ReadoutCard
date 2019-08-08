// Copyright CERN and copyright holders of ALICE O3. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

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
#include "RocPciDevice.h"
#endif

namespace AliceO2
{
namespace roc
{
namespace ChannelFactoryUtils
{

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
    for (const auto& c : cardsFound) {
      pciIds.push_back(c.pciId);
    }
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
    for (const auto& c : cardsFound) {
      pciIds.push_back(c.pciId);
    }
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Found more than one card with the given PCI address number")
                          << ErrorInfo::PciAddress(address)
                          << ErrorInfo::PciIds(pciIds));
  }

  return cardsFound.at(0);
}

inline CardDescriptor findCard(const PciSequenceNumber& sequenceNumber)
{
  auto cardsFound = RocPciDevice::findSystemDevices(sequenceNumber);

  if (cardsFound.empty()) {
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Could not find a card with the given PCI sequence Number")
                          << ErrorInfo::PciSequenceNumber(sequenceNumber));
  }

  if (cardsFound.size() > 1) {
    std::vector<PciId> pciIds;
    for (const auto& c : cardsFound) {
      pciIds.push_back(c.pciId);
    }
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Found more than one card with the given PCI sequence number")
                          << ErrorInfo::PciSequenceNumber(sequenceNumber)
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
  } else if (auto sequenceNumberStringMaybe = boost::get<PciSequenceNumber>(&id)) {
    auto sequenceNumber = *sequenceNumberStringMaybe;
    return findCard(sequenceNumber);
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Invalid Card ID"));
  }
}

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
#else

template <typename Interface>
std::unique_ptr<Interface> channelFactoryHelper(const Parameters& /*params*/, int /*dummySerial*/,
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

  // If PDA is NOT enabled we only make dummies
  return makeDummy();

#endif

}

} // namespace ChannelFactoryUtils
} // namespace roc
} // namespace AliceO2
