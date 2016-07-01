///
/// \file PdaDevice.h
/// \author Pascal Boeschoten
///

#pragma once

#include <string>
#include <boost/filesystem/path.hpp>
#include <pda.h>

namespace AliceO2 {
namespace Rorc {

/// Handles the creation and cleanup of the PDA DeviceOperator and PciDevice objects
class PdaDevice
{
  public:
    struct CardType
    {
        enum type
        {
          CRORC, CRU
        };
    };

    PdaDevice(int serialNumber);
    ~PdaDevice();
    DeviceOperator* getDeviceOperator();
    PciDevice* getPciDevice();

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

  private:
    std::string fileToString(const boost::filesystem::path& path);
    void newDeviceOperator(const std::string& vendorId, const std::string& deviceId);
    void getPciDevice(int serialNumber);

    DeviceOperator* deviceOperator;
    PciDevice* pciDevice;
    std::string pciDeviceId;
    std::string pciVendorId;
    CardType::type cardType;
};

} // namespace Rorc
} // namespace AliceO2
