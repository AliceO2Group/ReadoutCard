/// \file Numa.h
/// \brief Implementation of functions for NUMA
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "RORC/ParameterTypes/PciAddress.h"

namespace AliceO2 {
namespace Rorc {
namespace Utilities {

int getNumaNode(const PciAddress& pciAddress);

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
