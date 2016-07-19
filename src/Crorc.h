///
/// \file Crorc.h
/// \author Pascal Boeschoten
///
/// Low level C-RORC functions
///

#pragma once

#include <string>
#include <memory>
#include "PdaDevice.h"
#include "RORC/CardType.h"

namespace AliceO2 {
namespace Rorc {
namespace Crorc {

int getSerial(volatile void* barAddress);

} // namespace Crorc
} // namespace Rorc
} // namespace AliceO2
