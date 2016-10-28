/// \file CruChannelSlave.cxx
/// \brief Implementation of the CruChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruChannelSlave.h"

namespace AliceO2 {
namespace Rorc {

CruChannelSlave::CruChannelSlave(const Parameters& parameters)
    : ChannelSlave(parameters)
{
}

CruChannelSlave::~CruChannelSlave()
{
}

CardType::type CruChannelSlave::getCardType()
{
  return CardType::Cru;
}

} // namespace Rorc
} // namespace AliceO2
