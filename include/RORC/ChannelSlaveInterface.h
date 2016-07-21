///
/// \file ChannelSlaveInterface.h
/// \author Pascal Boeschoten
///

#pragma once

#include <cstdint>
#include "RORC/CardType.h"

namespace AliceO2 {
namespace Rorc {

/// Provides a limited-access interface to a RORC channel
/// TODO
///   - Register access restricted
///   - Possibly read-only access to pages?
class ChannelSlaveInterface
{
  public:
    virtual ~ChannelSlaveInterface()
    {
    }

    /// Reads a BAR register. The registers are indexed per 32 bits
    /// \param index The index of the register
    virtual uint32_t readRegister(int index) = 0;

    /// Writes a BAR register
    /// \param index The index of the register
    /// \param value The value to be written into the register
    virtual void writeRegister(int index, uint32_t value) = 0;

    /// Return the type of the RORC card this ChannelSlave is controlling
    /// \return The card type
    virtual CardType::type getCardType() = 0;
};

} // namespace Rorc
} // namespace AliceO2
