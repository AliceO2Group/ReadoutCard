///
/// \file ChannelFactory.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "RORC/ChannelFactory.h"
#include <map>
#include "RORC/CardType.h"
#include "RorcException.h"
#include "Dummy/DummyChannelMaster.h"
#include "Dummy/DummyChannelSlave.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
#  include <pda.h>
#  include "Crorc/CrorcChannelMaster.h"
#  include "Crorc/CrorcChannelSlave.h"
#  include "Cru/CruChannelMaster.h"
#  include "Cru/CruChannelSlave.h"
#else
#  pragma message("PDA not enabled, ChannelFactory will always return a dummy implementation")
#endif
#include "Factory/ChannelFactoryUtils.h"
#include <mutex>
#include <boost/thread/lock_guard.hpp>

namespace AliceO2 {
namespace Rorc {

using namespace FactoryHelper;
using namespace CardTypeTag;

ChannelFactory::ChannelFactory()
{
}

ChannelFactory::~ChannelFactory()
{
}

std::shared_ptr<ChannelMasterInterface> ChannelFactory::getMaster(int serial, int channel)
{
  return makeChannel<ChannelMasterInterface>(serial, DUMMY_SERIAL_NUMBER,
    DummyTag, [&](){ return std::make_shared<DummyChannelMaster>(serial, channel, ChannelParameters()); },
    CrorcTag, [&](){ return std::make_shared<CrorcChannelMaster>(serial, channel); },
    CruTag,   [&](){ return std::make_shared<CruChannelMaster>(serial, channel); });
}

std::shared_ptr<ChannelMasterInterface> ChannelFactory::getMaster(int serial, int channel,
    const ChannelParameters& params)
{
  return makeChannel<ChannelMasterInterface>(serial, DUMMY_SERIAL_NUMBER,
    DummyTag, [&](){ return std::make_shared<DummyChannelMaster>(serial, channel, params); },
    CrorcTag, [&](){ return std::make_shared<CrorcChannelMaster>(serial, channel, params); },
    CruTag,   [&](){ return std::make_shared<CruChannelMaster>(serial, channel, params); });
}


std::shared_ptr<ChannelSlaveInterface> ChannelFactory::getSlave(int serial, int channel)
{
  return makeChannel<ChannelSlaveInterface>(serial, DUMMY_SERIAL_NUMBER,
    DummyTag, [&](){ return std::make_shared<DummyChannelSlave>(serial, channel); },
    CrorcTag, [&](){ return std::make_shared<CrorcChannelSlave>(serial, channel); },
    CruTag,   [&](){ return std::make_shared<CruChannelSlave>(serial, channel); });
}

} // namespace Rorc
} // namespace AliceO2
