///
/// \file PdaDevice.h
/// \author Pascal Boeschoten
///

#pragma once

#include <string>
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

    PdaDevice(std::string vendorId, std::string deviceId);
    ~PdaDevice();
    DeviceOperator* getDeviceOperator();
    PciDevice* getPciDevice();

  private:

    DeviceOperator* deviceOperator;
    PciDevice* pciDevice;
};

} // namespace Rorc
} // namespace AliceO2
