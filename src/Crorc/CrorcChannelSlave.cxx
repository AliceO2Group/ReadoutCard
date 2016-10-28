/// \file CrorcChannelSlave.cxx
/// \brief Implementation of the CrorcChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Crorc/CrorcChannelSlave.h"

namespace AliceO2 {
namespace Rorc {

CrorcChannelSlave::CrorcChannelSlave(const Parameters& parameters)
    : ChannelSlave(parameters)
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
