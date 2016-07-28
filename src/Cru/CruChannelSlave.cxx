///
/// \file CruChannelSlave.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "CruChannelSlave.h"

namespace AliceO2 {
namespace Rorc {

CruChannelSlave::CruChannelSlave(int serial, int channel)
    : ChannelSlave(serial, channel)
{
}

CruChannelSlave::~CruChannelSlave()
{
}

CardType::type CruChannelSlave::getCardType()
{
  return CardType::CRU;
}

} // namespace Rorc
} // namespace AliceO2
