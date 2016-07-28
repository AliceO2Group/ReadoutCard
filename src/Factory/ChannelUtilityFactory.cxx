///
/// \file ChannelUtilityFactory.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "ChannelUtilityFactory.h"
#include <map>
#include "RORC/CardType.h"
#include "DummyChannelMaster.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
#  include "CrorcChannelMaster.h"
#  include "CruChannelMaster.h"
#else
#  pragma message("PDA not enabled, ChannelUtilityFactory will always return a dummy implementation")
#endif
#include "ChannelFactoryUtils.h"

namespace AliceO2 {
namespace Rorc {

ChannelUtilityFactory::ChannelUtilityFactory()
{
}

ChannelUtilityFactory::~ChannelUtilityFactory()
{
}

std::shared_ptr<ChannelUtilityInterface> ChannelUtilityFactory::getUtility(int serial, int channel)
{
  return channelFactoryHelper<ChannelUtilityInterface>(serial, DUMMY_SERIAL_NUMBER, {
    {CardType::DUMMY, [&](){ return std::make_shared<DummyChannelMaster>(serial, channel, ChannelParameters()); }},
    {CardType::CRORC, [&](){ return std::make_shared<CrorcChannelMaster>(serial, channel); }},
    {CardType::CRU,   [&](){ return std::make_shared<CruChannelMaster>(serial, channel); }}
  });
}

} // namespace Rorc
} // namespace AliceO2
