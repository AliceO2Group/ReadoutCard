///
/// \file PciId.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once
#include <string>

namespace AliceO2 {
namespace Rorc {

/// Simple data holder class for a PCI ID, consisting of a device ID and vendor ID.
struct PciId
{
    const std::string& getDeviceId() const
    {
      return device;
    }

    const std::string& getVendorId() const
    {
      return vendor;
    }

    std::string device;
    std::string vendor;
};

} // namespace Rorc
} // namespace AliceO2
