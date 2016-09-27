/// \file PdaDevice.h
/// \brief Definition of the PdaDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <string>
#include <vector>
#include <pda.h>
#include "RORC/PciId.h"

namespace AliceO2 {
namespace Rorc {
namespace Pda {

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
      return mPciDevices;
    }

  private:

    DeviceOperator* mDeviceOperator;
    std::vector<PciDevice*> mPciDevices;
};

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
