///
/// \file CruChannelSlave.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "ChannelSlave.h"

namespace AliceO2 {
namespace Rorc {

/// TODO Currently a placeholder
class CruChannelSlave: public ChannelSlave
{
  public:

    CruChannelSlave(int serial, int channel);
    virtual ~CruChannelSlave();
    virtual CardType::type getCardType();
};

} // namespace Rorc
} // namespace AliceO2
