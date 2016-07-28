///
/// \file ChannelFactoryUtils.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include <map>
#include "RORC/CardType.h"
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

/// Helper template method for the channel factories.
/// \param serialNumber Serial number of the card
/// \param dummySerial Serial number that indicates a dummy object should be instantiated
/// \param map A mapping of CardType to functions that create a shared pointer to an implementation of the Interface
template <typename Interface>
std::shared_ptr<Interface> channelFactoryHelper(int serialNumber, int dummySerial,
    const std::map<CardType::type, std::function<std::shared_ptr<Interface>()>>& map)
{
#ifdef ALICEO2_RORC_PDA_ENABLED
  if (serialNumber == dummySerial) {
    return map.at(CardType::DUMMY)();
  } else {
    // Find the PCI device
    auto cardsFound = RorcDevice::enumerateDevices(serialNumber);
    if (cardsFound.empty()) {
      BOOST_THROW_EXCEPTION(RorcException()
          << errinfo_rorc_generic_message("Could not find card")
          << errinfo_rorc_serial_number(serialNumber));
    }
    auto& card = cardsFound[0];
    return map.at(card.cardType)();
    BOOST_THROW_EXCEPTION(RorcException()
        << errinfo_rorc_generic_message("Unknown card type")
        << errinfo_rorc_serial_number(serialNumber));

    return std::shared_ptr<Interface>(nullptr);
  }
#else
  return makeDummy();
#endif
}

} // namespace Rorc
} // namespace AliceO2
