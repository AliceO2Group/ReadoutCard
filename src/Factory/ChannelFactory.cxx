/// \file ChannelFactory.cxx
/// \brief Implementation of the ChannelFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ChannelFactory.h"
#include "Dummy/DummyDmaChannel.h"
#include "Dummy/DummyBar.h"
#include "Factory/ChannelFactoryUtils.h"
#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
#  include "Crorc/CrorcDmaChannel.h"
#  include "Crorc/CrorcBar.h"
#  include "Cru/CruDmaChannel.h"
#  include "Cru/CruBar.h"
#else
#  pragma message("PDA not enabled, ChannelFactory will always return a dummy implementation")
#endif

namespace AliceO2 {
namespace roc {

using namespace FactoryHelper;
using namespace CardTypeTag;

ChannelFactory::ChannelFactory()
{
}

ChannelFactory::~ChannelFactory()
{
}

auto ChannelFactory::getDmaChannel(const Parameters &params) -> DmaChannelSharedPtr
{
  return makeChannel<DmaChannelInterface>(params, getDummySerialNumber()
    , DummyTag, [&]{ return std::make_shared<DummyDmaChannel>(params); }
#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
    , CrorcTag, [&]{ return std::make_shared<CrorcDmaChannel>(params); }
    , CruTag,   [&]{ return std::make_shared<CruDmaChannel>(params); }
#endif
    );
}

auto ChannelFactory::getBar(const Parameters &params) -> BarSharedPtr
{
  return makeChannel<BarInterface>(params, getDummySerialNumber()
    , DummyTag, [&]{ return std::make_shared<DummyBar>(params); }
#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
    , CrorcTag, [&]{ return std::make_shared<CrorcBar>(params); }
    , CruTag,   [&]{ return std::make_shared<CruBar>(params); }
#endif
    );
}

} // namespace roc
} // namespace AliceO2
