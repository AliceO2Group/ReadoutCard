/// \file Numa.h
/// \brief Implementation of functions for NUMA
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_UTILITIES_NUMA_H_
#define ALICEO2_SRC_READOUTCARD_UTILITIES_NUMA_H_

#include "ReadoutCard/ParameterTypes/PciAddress.h"

namespace AliceO2 {
namespace roc {
namespace Utilities {

int getNumaNode(const PciAddress& pciAddress);

} // namespace Util
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_UTILITIES_NUMA_H_