///
/// \file RegisterReadWriteInterface.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <cstdint>

namespace AliceO2 {
namespace Rorc {

class RegisterReadWriteInterface
{
  public:
    virtual ~RegisterReadWriteInterface()
    {
    }

    /// Reads a BAR register. The registers are indexed per 32 bits
    /// \param index The index of the register
    virtual uint32_t readRegister(int index) = 0;

    /// Writes a BAR register
    /// \param index The index of the register
    /// \param value The value to be written into the register
    virtual void writeRegister(int index, uint32_t value) = 0;
};

} // namespace Rorc
} // namespace AliceO2
