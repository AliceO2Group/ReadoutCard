///
/// \file PdaDevice.h
/// \author Pascal Boeschoten
///

#pragma once

#include <string>
#include <vector>
#include <pda.h>

namespace AliceO2 {
namespace Rorc {

/// Handles the creation and cleanup of the PDA DeviceOperator and PciDevice objects
class PdaDevice
{
  public:
    PdaDevice(const std::string& vendorId, const std::string& deviceId);
    ~PdaDevice();
    DeviceOperator* getDeviceOperator();

    int getPciDeviceCount();
    PciDevice* getPciDevice(int index);

    const std::vector<PciDevice*>& getPciDevices() const
    {
      return pciDevices;
    }

  private:

    DeviceOperator* deviceOperator;
    std::vector<PciDevice*> pciDevices;
};

} // namespace Rorc
} // namespace AliceO2
