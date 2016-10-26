/// \file ChannelUtilityFactory.cxx
/// \brief Implementation of the ChannelUtilityFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Factory/ChannelUtilityFactory.h"
#include <map>
#include "RORC/CardType.h"
#include "Dummy/DummyChannelMaster.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
#  include "Crorc/CrorcChannelMaster.h"
#  include "Cru/CruChannelMaster.h"
#else
#  pragma message("PDA not enabled, ChannelUtilityFactory will always return a dummy implementation")
#endif
#include "Factory/ChannelFactoryUtils.h"

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

auto ChannelUtilityFactory::getUtility(int serial, int channel) -> UtilitySharedPtr
{
  return makeChannel<ChannelUtilityInterface>(serial, DUMMY_SERIAL_NUMBER
    , DummyTag, [&]{ return std::make_shared<DummyChannelMaster>(serial, channel, Parameters::Map()); }
#ifdef ALICEO2_RORC_PDA_ENABLED
    , CrorcTag, [&]{ return std::make_shared<CrorcChannelMaster>(serial, channel, Parameters::Map()); }
    , CruTag,   [&]{ return std::make_shared<CruChannelMaster>(serial, channel, Parameters::Map()); }
#endif
    );
}

} // namespace Rorc
} // namespace AliceO2
