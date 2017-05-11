/// \file CruChannelSlave.h
/// \brief Definition of the CruChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ChannelSlave.h"

namespace AliceO2 {
namespace roc {

/// TODO Currently a placeholder
class CruChannelSlave: public ChannelSlave
{
  public:

    CruChannelSlave(const Parameters& parameters);
    virtual ~CruChannelSlave();
    virtual CardType::type getCardType();
};

} // namespace roc
} // namespace AliceO2
