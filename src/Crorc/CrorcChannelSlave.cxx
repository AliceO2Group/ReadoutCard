/// \file CrorcChannelSlave.cxx
/// \brief Implementation of the CrorcChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

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
  return CardType::Crorc;
}

} // namespace Rorc
} // namespace AliceO2
