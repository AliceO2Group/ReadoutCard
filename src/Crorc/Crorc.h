/// \file Crorc.h
/// \brief Definition of low level C-RORC functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include "RORC/RegisterReadWriteInterface.h"

namespace AliceO2 {
namespace Rorc {
namespace Crorc {

/// Retrieve the serial number from the C-RORC's flash memory.
/// Note that the BAR must be from channel 0, other channels do not have access to the flash.
int getSerial(volatile void* barAddress);

/// Program flash using given data file
void programFlash(RegisterReadWriteInterface& channel, std::string dataFilePath, int addressFlash, std::ostream& out,
    const std::atomic<bool>* interrupt = nullptr);

/// Read flash range
void readFlashRange(RegisterReadWriteInterface& channel, int addressFlash, int wordNumber, std::ostream& out);

} // namespace Crorc
} // namespace Rorc
} // namespace AliceO2
