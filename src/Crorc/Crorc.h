/// \file Crorc.h
/// \brief Definition of low level C-RORC functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstdint>

namespace AliceO2 {
namespace Rorc {
namespace Crorc {

/// Retrieve the serial number from the C-RORC's flash memory.
/// Note that the BAR must be from channel 0, other channels do not have access to the flash.
int getSerial(uintptr_t barAddress);

} // namespace Crorc
} // namespace Rorc
} // namespace AliceO2
