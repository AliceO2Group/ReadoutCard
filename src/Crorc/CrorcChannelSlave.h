///
/// \file CrorcChannelSlave.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "ChannelSlave.h"

namespace AliceO2 {
namespace Rorc {

/// TODO Currently a placeholder
class CrorcChannelSlave: public ChannelSlave
{
  public:

    CrorcChannelSlave(int serial, int channel);
    virtual ~CrorcChannelSlave();
    virtual CardType::type getCardType();
};

} // namespace Rorc
} // namespace AliceO2
