/// \file ChannelFactoryUtils.h
/// \brief Definition of helper functions for Channel factories
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <map>
#include <algorithm>
#include "RORC/CardType.h"
#include "RORC/Exception.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
# include "RorcDevice.h"
#endif

namespace AliceO2 {
namespace Rorc {

namespace FactoryHelper {

#ifdef ALICEO2_RORC_PDA_ENABLED
inline RorcDevice::CardDescriptor findCard(int serial)
{
  auto cardsFound = RorcDevice::findSystemDevices(serial);

  if (cardsFound.empty()) {
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("Could not find a card with the given serial number"));
  }

  if (cardsFound.size() > 1) {
    std::vector<PciId> pciIds;
    for (const auto& c : cardsFound) { pciIds.push_back(c.pciId); }
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("Found more than one card with the given serial number")
        << errinfo_rorc_pci_ids(pciIds));
  }

  return cardsFound.at(0);
}
#endif

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

    auto card = findCard(serialNumber);

    if (map.count(card.cardType) == 0) {
      BOOST_THROW_EXCEPTION(Exception()
          << errinfo_rorc_error_message("Unknown card type")
          << errinfo_rorc_serial_number(serialNumber)
          << errinfo_rorc_card_type(card.cardType));
    }

    return map.at(card.cardType)();
  }
#else
  return map.at(CardType::Dummy)();
#endif
}

/// "private" namespace for the implementation of makeChannel()
namespace _make_impl {

/// Declaration for the makeChannel implementation struct
template <class Result, int Index, class... Args>
struct Make;

/// Stop condition implementation
template <class Result, int Index>
struct Make<Result, Index>
{
  static Result make(CardType::type cardType)
  {
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("No card match found")
        << errinfo_rorc_card_type(cardType));
  }
};

/// Recursive step implementation
template <class Result, int Index, class Tag, class Function, class... Args>
struct Make<Result, Index, Tag, Function, Args...>
{
  /// Pops two arguments from the front of the Args, a CardTypeTag and an accompanying instantiation function, and
  /// tries to match the tag with the 'select' argument. If there's no match, it recursively goes down the list of
  /// arguments until it either finds the correct tag, or until it runs out of pairs and thus hits the stop condition
  /// implementation
  static Result make(CardType::type select, Tag typeTag, Function func, Args&&... args)
  {
    // Make sure this tag is valid and not a dummy
    static_assert(CardTypeTag::isValidTag<Tag>(), "CardTypeTag of pair is not a valid tag");
    static_assert((Index == 0 && CardTypeTag::isDummyTag<Tag>()) || Index != 0,
       "DummyTag & accompanying function must be in first position");
    static_assert((Index != 0 && CardTypeTag::isNonDummyTag<Tag>()) || Index == 0,
       "DummyTag & accompanying function may be in first position only");
 
    // If we find the right tag, instantiate using accompanying function, else recursively go down the argument list
    return (typeTag.type == select) ? func() : Make<Result, Index + 1, Args...>::make(select, std::forward<Args>(args)...);
  }

  static Result makeDummy(Tag, Function func, Args&&...)
  {
    static_assert(Index == 0, "DummyTag & accompanying function must be in first position");
    static_assert(std::is_same<typename std::decay<Tag>::type, CardTypeTag::DummyTag_>::value,
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
/// \tparam Interface The pointer type of the returned smart_ptr
/// \param serial Serial number of the card
/// \param dummySerial Serial number that indicates a dummy object should be instantiated
/// \param args A mapping of CardType to functions that create a shared pointer to an implementation of the Interface.
///   Formed by pairs of arguments of a CardTypeTag tag and a callable object. The first mapping must use DummyTag.
///   Example:
///   auto sharedPointer = make<ChannelMasterInterface>(12345, -1,
///       CardTypeTag::DummyTag, []{ return makeDummy(); },
///       CardTypeTag::CrorcTag, []{ return makeCrorc(); });
/// \return A shared pointer to the constructed channel
template<class Interface, class... Args>
std::shared_ptr<Interface> makeChannel(int serial, int dummySerial, Args&&... args)
{
  using SharedPtr = std::shared_ptr<Interface>;
  using namespace _make_impl;

  auto makeDummy = [&]{ return Make<SharedPtr, 0, Args...>::makeDummy(std::forward<Args>(args)...); };

  try {
#ifndef ALICEO2_RORC_PDA_ENABLED
    // If PDA is NOT enabled we only make dummies
    return makeDummy();
#else
    // If PDA IS enabled we can make a dummy or a real channel
    auto makeReal = [&](auto cardType){ return Make<SharedPtr, 0, Args...>::make(cardType, std::forward<Args>(args)...); };
    return (dummySerial == serial) ? makeDummy() : makeReal(findCard(serial).cardType);
#endif
  } catch (boost::exception& e) {
    e << errinfo_rorc_serial_number(serial);
    throw;
  }
}
} // namespace FactoryHelper

} // namespace Rorc
} // namespace AliceO2
