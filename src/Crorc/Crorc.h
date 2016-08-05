///
/// \file Crorc.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Low level C-RORC functions
///

#pragma once

namespace AliceO2 {
namespace Rorc {
namespace Crorc {

/// Retrieve the serial number from the C-RORC's flash memory.
/// Note that the BAR must be from channel 0, other channels do not have access to the flash.
int getSerial(volatile void* barAddress);

} // namespace Crorc
} // namespace Rorc
} // namespace AliceO2
