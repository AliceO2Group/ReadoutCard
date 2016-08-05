///
/// \file CrorcChannelSlave.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "Crorc/CrorcChannelSlave.h"

namespace AliceO2 {
namespace Rorc {

CrorcChannelSlave::CrorcChannelSlave(int serial, int channel)
    : ChannelSlave(serial, channel)
{
}

CrorcChannelSlave::~CrorcChannelSlave()
{
}

CardType::type CrorcChannelSlave::getCardType()
{
  return CardType::CRORC;
}

} // namespace Rorc
} // namespace AliceO2
