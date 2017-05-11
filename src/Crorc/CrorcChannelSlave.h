/// \file CrorcChannelSlave.h
/// \brief Definition of the CrorcChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ChannelSlave.h"

namespace AliceO2 {
namespace roc {

/// TODO Currently a placeholder
class CrorcChannelSlave: public ChannelSlave
{
  public:

    CrorcChannelSlave(const Parameters& parameters);
    virtual ~CrorcChannelSlave();
    virtual CardType::type getCardType();
};

} // namespace roc
} // namespace AliceO2
