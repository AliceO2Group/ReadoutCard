/// \file ChannelFactoryUtils.h
/// \brief Definition of helper functions for Channel factories
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <map>
#include <algorithm>
#include "RORC/CardType.h"
#include "RORC/Exception.h"
#include "RorcDevice.h"

namespace AliceO2 {
namespace Rorc {

namespace FactoryHelper {

/// Helper template method for the channel factories.
/// \param serialNumber Serial number of the card
/// \param dummySerial Serial number that indicates a dummy object should be instantiated
/// \param map A mapping of CardType to functions that create a shared pointer to an implementation of the Interface
///   Formed by pairs of arguments of a CardType and an std::function. The first mapping must use Dummy.
///   Example:
///   auto sharedPointer = channelFactoryHelper<ChannelMasterInterface>(12345, -1, {
///       { CardType::DUMMY, []{ return makeDummy(); }},
///       { CardType::CRORC, []{ return makeCrorc(); }}});
template <typename Interface>
std::shared_ptr<Interface> channelFactoryHelper(int serialNumber, int dummySerial,
    const std::map<CardType::type, std::function<std::shared_ptr<Interface>()>>& map)
{
#ifdef ALICEO2_RORC_PDA_ENABLED
  if (serialNumber == dummySerial) {
    return map.at(CardType::Dummy)();
  } else {
    // Find the PCI device
    auto cardsFound = RorcDevice::findSystemDevices(serialNumber);

    if (cardsFound.empty()) {
      BOOST_THROW_EXCEPTION(Exception()
          << errinfo_rorc_error_message("Could not find card")
          << errinfo_rorc_serial_number(serialNumber));
    }

    if (cardsFound.size() > 1) {
      BOOST_THROW_EXCEPTION(Exception()
          << errinfo_rorc_error_message("Found multiple cards with the same serial number")
          << errinfo_rorc_serial_number(serialNumber));
    }

    auto cardType = cardsFound[0].cardType;

    if (map.count(cardType) == 0) {
      BOOST_THROW_EXCEPTION(Exception()
          << errinfo_rorc_error_message("Unknown card type")
          << errinfo_rorc_serial_number(serialNumber)
          << errinfo_rorc_card_type(cardType));
    }

    return map.at(cardType)();
  }
#else
  return makeDummy();
#endif
}

/// "private" namespace for the implementation of makeChannel()
namespace _make_impl {

/// Declaration for the makeChannel implementation struct
template <class... Args> struct MakeImpl;

/// Stop condition implementation
template <class Result>
struct MakeImpl<Result>
{
  static Result make(CardType::type cardType)
  {
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("No card match found")
        << errinfo_rorc_card_type(cardType));
  }
};

/// Recursive step implementation
template <class Result, class Tag, class Function, class... Args>
struct MakeImpl<Result, Tag, Function, Args...>
{
  /// Pops two arguments from the front of the Args, a CardTypeTag and an accompanying instantiation function, and
  /// tries to match the tag with the 'select' argument. If there's no match, it recursively goes down the list of
  /// arguments until it either finds the correct tag, or until it runs out of pairs and thus hits the stop condition
  /// implementation
  static Result make(CardType::type select, Tag typeTag, Function func, Args&&... args)
  {
    static_assert(CardTypeTag::isValidTag<Tag>(), "CType is not a valid tag");

    if (typeTag.type == select) {
      // Found the right tag, instantiate using accompanying function
      return func();
    } else {
      // Recursively go down the argument list
      return MakeImpl<Result, Args...>::make(select, std::forward<Args>(args)...);
    }
  }

  /// Implementation for a dummy instantiation. It is assumed the first pair of tag & function in the Args... are
  /// the dummy ones
  static Result makeDummy(Tag, Function func, Args&&...)
  {
    static_assert(std::is_same<Tag, const decltype(CardTypeTag::DummyTag)&>::value,
        "DummyTag & accompanying function must be in first position");
    return func();
  }
};
} // namespace _make_impl

/// Helper template method for the channel factories. If serial == dummySerial, the dummy will be instantiated.
/// If ALICEO2_RORC_PDA_ENABLED is not defined, a dummy will always be instantiated.
/// It does essentially the same thing as channelFactoryHelper, except with:
///  - More checks and work done at compile time (an invalid tag will cause a compile error instead of a runtime error)
///  - Less curly braces.
///
/// Is that worth the ugliness of the template implementation? Good question...
///
/// \param Interface The pointer type of the returned smart_ptr
/// \param dummySerial Serial number that indicates a dummy object should be instantiated
/// \param serialNumber Serial number of the card
/// \param args A mapping of CardType to functions that create a shared pointer to an implementation of the Interface.
///   Formed by pairs of arguments of a CardTypeTag tag and a callable object. The first mapping must use DummyTag.
///   Example:
///   auto sharedPointer = make<ChannelMasterInterface>(12345, -1,
///       CardTypeTag::DummyTag, []{ return makeDummy(); },
///       CardTypeTag::CrorcTag, []{ return makeCrorc(); });
///
template<class Interface, class ... Args>
std::shared_ptr<Interface> makeChannel(int serial, int dummySerial, Args&&... args)
{
  using SharedPtr = std::shared_ptr<Interface>;
  try {
#ifdef ALICEO2_RORC_PDA_ENABLED
    if (dummySerial == serial) {
#endif
      return _make_impl::MakeImpl<SharedPtr, Args...>::makeDummy(std::forward<Args>(args)...);
#ifdef ALICEO2_RORC_PDA_ENABLED
    }

    auto cardsFound = RorcDevice::findSystemDevices(serial);

    if (cardsFound.empty()) {
      BOOST_THROW_EXCEPTION(Exception()
          << errinfo_rorc_error_message("Could not find a card with the given serial number"));
    }

    if (cardsFound.size() > 1) {
      std::vector<PciId> pciIds;
      for (auto& c : cardsFound) { pciIds.push_back(c.pciId); }
      BOOST_THROW_EXCEPTION(Exception()
          << errinfo_rorc_error_message("Found more than one card with the given serial number")
          << errinfo_rorc_pci_ids(pciIds));
    }

    auto cardType = cardsFound[0].cardType;

    return _make_impl::MakeImpl<SharedPtr, Args...>::make(cardType, std::forward<Args>(args)...);
#endif
  } catch (boost::exception& e) {
    e << errinfo_rorc_serial_number(serial);
    throw;
  }
}
} // namespace FactoryHelper

} // namespace Rorc
} // namespace AliceO2
