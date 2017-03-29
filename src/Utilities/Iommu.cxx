/// \file Iommu.cxx
/// \brief Implementation of functions related to the IOMMU
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "Iommu.h"
#include <boost/filesystem/operations.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Utilities {
namespace Iommu {
namespace {
} // Anonymous namespace

bool isEnabled()
{
  // This should do the trick...
  return boost::filesystem::exists("/sys/kernel/iommu_groups/0");
}

} // namespace Iommu
} // namespace Util
} // namespace Rorc
} // namespace AliceO2
