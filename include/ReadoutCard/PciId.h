/// \file PciId.h
/// \brief Definition of the PciId struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_PCIID_H_
#define ALICEO2_INCLUDE_READOUTCARD_PCIID_H_

#include <string>

namespace AliceO2 {
namespace roc {

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

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_PCIID_H_
