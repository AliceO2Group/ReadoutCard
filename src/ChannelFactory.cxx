///
/// \file ChannelFactory.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "RORC/ChannelFactory.h"
#include <dirent.h>
#include <map>
//#include <boost/range/iterator_range.hpp>
//#include <boost/interprocess/sync/file_lock.hpp>
//#include <boost/format.hpp>
//#include <boost/filesystem.hpp>
//#include <boost/filesystem/fstream.hpp>
#include "RORC/CardType.h"
#include "RorcException.h"
#include "DummyChannelMaster.h"
#include "DummyChannelSlave.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
#  include <pda.h>
#  include "CrorcChannelMaster.h"
#  include "CrorcChannelSlave.h"
#  include "CruChannelMaster.h"
#  include "CruChannelSlave.h"
#  include "ChannelPaths.h"
#  include "RorcDevice.h"
#else
#  pragma message("PDA not enabled, ChannelFactory will always return a dummy implementation")
#endif

namespace AliceO2 {
namespace Rorc {

ChannelFactory::ChannelFactory()
{
}

ChannelFactory::~ChannelFactory()
{
}

namespace {

template <typename Interface>
std::shared_ptr<Interface> channelFactoryHelper(int serialNumber,
    std::map<CardType::type, std::function<std::shared_ptr<Interface>()>> map)
{
#ifdef ALICEO2_RORC_PDA_ENABLED
  if (serialNumber == ChannelFactory::DUMMY_SERIAL_NUMBER) {
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

} // Anonymous namespace

std::shared_ptr<ChannelMasterInterface> ChannelFactory::getMaster(int serial, int channel)
{
  return channelFactoryHelper<ChannelMasterInterface>(serial, {
    {CardType::DUMMY, [&](){ return std::make_shared<DummyChannelMaster>(serial, channel, ChannelParameters()); }},
    {CardType::CRORC, [&](){ return std::make_shared<CrorcChannelMaster>(serial, channel); }},
    {CardType::CRU,   [&](){ return std::make_shared<CruChannelMaster>(serial, channel); }}
  });
}

std::shared_ptr<ChannelMasterInterface> ChannelFactory::getMaster(int serial, int channel,
    const ChannelParameters& params)
{
  return channelFactoryHelper<ChannelMasterInterface>(serial, {
    {CardType::DUMMY, [&](){ return std::make_shared<DummyChannelMaster>(serial, channel, params); }},
    {CardType::CRORC, [&](){ return std::make_shared<CrorcChannelMaster>(serial, channel, params); }},
    {CardType::CRU,   [&](){ return std::make_shared<CruChannelMaster>(serial, channel, params); }}
  });
}


std::shared_ptr<ChannelSlaveInterface> ChannelFactory::getSlave(int serial, int channel)
{
  return channelFactoryHelper<ChannelSlaveInterface>(serial, {
    {CardType::DUMMY, [&](){ return std::make_shared<DummyChannelSlave>(serial, channel); }},
    {CardType::CRORC, [&](){ return std::make_shared<CrorcChannelSlave>(serial, channel); }},
    {CardType::CRU,   [&](){ return std::make_shared<CruChannelSlave>(serial, channel); }}
  });
}

} // namespace Rorc
} // namespace AliceO2
