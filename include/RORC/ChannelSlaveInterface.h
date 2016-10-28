/// \file ChannelSlaveInterface.h
/// \brief Definition of the ChannelSlaveInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstdint>
#include "RORC/CardType.h"
#include "RORC/RegisterReadWriteInterface.h"
#include "RORC/Parameters.h"

namespace AliceO2 {
namespace Rorc {

/// Provides a limited-access interface to a RORC channel
/// TODO
///   - Register access restricted
///   - Possibly read-only access to pages?
class ChannelSlaveInterface: public virtual RegisterReadWriteInterface
{
  public:
    virtual ~ChannelSlaveInterface()
    {
    }

    /// Return the type of the RORC card this ChannelSlave is controlling
    /// \return The card type
    virtual CardType::type getCardType() = 0;
};

} // namespace Rorc
} // namespace AliceO2
