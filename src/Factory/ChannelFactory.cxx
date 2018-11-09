/// \file ChannelFactory.cxx
/// \brief Implementation of the ChannelFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ReadoutCard.h"
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

using namespace ChannelFactoryUtils;
using namespace CardTypeTag;

ChannelFactory::ChannelFactory()
{
}

ChannelFactory::~ChannelFactory()
{
}

auto ChannelFactory::getDmaChannel(const Parameters &params) -> DmaChannelSharedPtr
{
  return channelFactoryHelper<DmaChannelInterface>(params, getDummySerialNumber(), {
    {CardType::Dummy, [&]{ return std::make_unique<DummyDmaChannel>(params); }},
#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
    {CardType::Crorc, [&]{ return std::make_unique<CrorcDmaChannel>(params); }},
    {CardType::Cru,   [&]{ return std::make_unique<CruDmaChannel>(params); }}
#endif
  });
}

auto ChannelFactory::getBar(const Parameters &params) -> BarSharedPtr
{
  return channelFactoryHelper<BarInterface>(params, getDummySerialNumber(), {
    {CardType::Dummy, [&]{ return std::make_unique<DummyBar>(params); }},
#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
    {CardType::Crorc, [&]{ return std::make_unique<CrorcBar>(params); }},
    {CardType::Cru,   [&]{ return std::make_unique<CruBar>(params); }}
#endif
  });
}

} // namespace roc
} // namespace AliceO2
