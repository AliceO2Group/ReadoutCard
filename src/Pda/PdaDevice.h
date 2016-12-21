/// \file PdaDevice.h
/// \brief Definition of the PdaDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
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
    using SharedPdaDevice = std::shared_ptr<PdaDevice>;

    ~PdaDevice();

    class PdaDeviceOperator
    {
      public:
        PdaDeviceOperator(DeviceOperator* deviceOperator, SharedPdaDevice pdaDevice)
            : mDeviceOperator(deviceOperator), mPdaDevice(pdaDevice)
        {
        }

        DeviceOperator* get() const
        {
          return mDeviceOperator;
        }
      private:
        DeviceOperator* mDeviceOperator;
        SharedPdaDevice mPdaDevice;
    };

    class PdaPciDevice
    {
      public:
        PdaPciDevice(PciDevice* pciDevice, SharedPdaDevice pdaDevice)
            : mPciDevice(pciDevice), mPdaDevice(pdaDevice)
        {
        }

        PciDevice* get() const
        {
          return mPciDevice;
        }
      private:
        PciDevice* mPciDevice;
        SharedPdaDevice mPdaDevice;
    };

    PdaDevice(const PciId& pciId);

    static SharedPdaDevice getPdaDevice(const PciId& pciId)
    {
      return std::make_shared<PdaDevice>(pciId);
    }

    static PdaDeviceOperator getDeviceOperator(SharedPdaDevice pdaDevice)
    {
      return {pdaDevice->mDeviceOperator, pdaDevice};
    }

    static std::vector<PdaPciDevice> getPciDevices(SharedPdaDevice pdaDevice)
    {
      std::vector<PdaPciDevice> devices;
      for (auto pciDevice : pdaDevice->mPciDevices) {
        devices.emplace_back(pciDevice, pdaDevice);
      }
      return devices;
    }

    static std::vector<PdaPciDevice> getPciDevices(const PciId& pciId)
    {
      auto pdaDevice = getPdaDevice(pciId);
      std::vector<PdaPciDevice> devices;
      for (auto pciDevice : pdaDevice->mPciDevices) {
        devices.emplace_back(pciDevice, pdaDevice);
      }
      return devices;
    }

  private:

    int getPciDeviceCount();
    PciDevice* getPciDevice(int index);

    DeviceOperator* mDeviceOperator;
    std::vector<PciDevice*> mPciDevices;
};

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
