///
/// \file RorcDeviceFinder.h
/// \author Pascal Boeschoten
///

#pragma once

#include <string>

namespace AliceO2 {
namespace Rorc {

/// Attempts to finds the RORC PCI device
class RorcDeviceFinder
{
  public:
    struct CardType
    {
        enum type
        {
          UNKNOWN, CRORC, CRU
        };
    };

    RorcDeviceFinder(int serialNumber);
    ~RorcDeviceFinder();

    const std::string& getPciDeviceId() const
    {
      return pciDeviceId;
    }

    const std::string& getPciVendorId() const
    {
      return pciVendorId;
    }

    CardType::type getCardType() const
    {
      return cardType;
    }

    int getSerialNumber() const
    {
      return rorcSerialNumber;
    }

  private:
    std::string pciDeviceId;
    std::string pciVendorId;
    int rorcSerialNumber;
    CardType::type cardType;
};

} // namespace Rorc
} // namespace AliceO2
