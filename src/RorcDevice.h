///
/// \file RorcDevice.h
/// \author Pascal Boeschoten
///

#pragma once

#include <string>
#include <memory>
#include "PdaDevice.h"
#include "RORC/CardType.h"

namespace AliceO2 {
namespace Rorc {

/// Represents a single RORC PCI device
/// Note: some duplication of code with RorcDeviceEnumerator TODO refactor a bit
class RorcDevice
{
  public:
    RorcDevice(int serialNumber);
    ~RorcDevice();

    const std::string& getDeviceId() const
    {
      return deviceId;
    }

    const std::string& getVendorId() const
    {
      return vendorId;
    }

    CardType::type getCardType() const
    {
      return cardType;
    }

    int getSerialNumber() const
    {
      return serialNumber;
    }

    PciDevice* getPciDevice() const
    {
      return pciDevice;
    }

    struct CardDescriptor
    {
        CardType::type cardType;
        int serialNumber;
        std::string deviceId;
        std::string vendorId;
    };

    // Finds RORC devices on the system
    static std::vector<CardDescriptor> enumerateDevices();

    // Finds RORC devices on the system with the given serial number
    static std::vector<CardDescriptor> enumerateDevices(int serialNumber);

  private:
    std::auto_ptr<PdaDevice> pdaDevice;
    PciDevice* pciDevice;
    std::string deviceId;
    std::string vendorId;
    int serialNumber;
    CardType::type cardType;
};

} // namespace Rorc
} // namespace AliceO2
