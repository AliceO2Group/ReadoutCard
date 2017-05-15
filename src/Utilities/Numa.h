/// \file Numa.h
/// \brief Implementation of functions for NUMA
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ReadoutCard/ParameterTypes/PciAddress.h"

namespace AliceO2 {
namespace roc {
namespace Utilities {

int getNumaNode(const PciAddress& pciAddress);

} // namespace Util
} // namespace roc
} // namespace AliceO2
