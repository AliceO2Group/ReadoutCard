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
using namespace FactoryHelper;
using namespace CardTypeTag;

ChannelUtilityFactory::ChannelUtilityFactory()
{
}

ChannelUtilityFactory::~ChannelUtilityFactory()
{
}

std::shared_ptr<ChannelUtilityInterface> ChannelUtilityFactory::getUtility(int serial, int channel)
{
  return makeChannel<ChannelUtilityInterface>(serial, DUMMY_SERIAL_NUMBER,
    DummyTag, [&](){ return std::make_shared<DummyChannelMaster>(serial, channel, ChannelParameters()); },
    CrorcTag, [&](){ return std::make_shared<CrorcChannelMaster>(serial, channel); },
    CruTag,   [&](){ return std::make_shared<CruChannelMaster>(serial, channel); });
}

} // namespace Rorc
} // namespace AliceO2
