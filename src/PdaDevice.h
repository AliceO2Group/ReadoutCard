///
/// \file PdaDevice.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <string>
#include <vector>
#include <pda.h>
#include "RORC/PciId.h"

namespace AliceO2 {
namespace Rorc {

/// Handles the creation and cleanup of the PDA DeviceOperator and PciDevice objects
class PdaDevice
{
  public:
    PdaDevice(const PciId& pciId);
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
