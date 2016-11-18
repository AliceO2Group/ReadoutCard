/// \file ChannelFactory.cxx
/// \brief Implementation of the ChannelFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/Parameters.h"
#include "RORC/ChannelFactory.h"
#include "RORC/CardType.h"
#include "RORC/ResetLevel.h"
#include "RORC/GeneratorPattern.h"
#include "RORC/LoopbackMode.h"
#include "RORC/Exception.h"
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

auto ChannelFactory::getMaster(const Parameters& params) -> MasterSharedPtr
{
  return makeChannel<ChannelMasterInterface>(params, DUMMY_SERIAL_NUMBER
    , DummyTag, [&]{ return std::make_shared<DummyChannelMaster>(params); }
#ifdef ALICEO2_RORC_PDA_ENABLED
    , CrorcTag, [&]{ return std::make_shared<CrorcChannelMaster>(params); }
    , CruTag,   [&]{ return std::make_shared<CruChannelMaster>(params); }
#endif
    );
}

auto ChannelFactory::getSlave(const Parameters& params) -> SlaveSharedPtr
{
  return makeChannel<ChannelSlaveInterface>(params, DUMMY_SERIAL_NUMBER
    , DummyTag, [&]{ return std::make_shared<DummyChannelSlave>(params); }
#ifdef ALICEO2_RORC_PDA_ENABLED
    , CrorcTag, [&]{ return std::make_shared<CrorcChannelSlave>(params); }
    , CruTag,   [&]{ return std::make_shared<CruChannelSlave>(params); }
#endif
    );
}

} // namespace Rorc
} // namespace AliceO2
