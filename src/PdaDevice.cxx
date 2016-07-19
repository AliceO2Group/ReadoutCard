///
/// \file PdaDevice.cxx
/// \author Pascal Boeschoten
///

#include "PdaDevice.h"
#include <map>
#include <pda.h>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

namespace b = boost;
namespace bfs = boost::filesystem;

PdaDevice::PdaDevice(const std::string& vendorId, const std::string& deviceId) : deviceOperator(nullptr)
{
  if (PDAInit() != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to initialize PDA");
  }

  if (deviceOperator != nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("DeviceOperator was already initialized");
  }

  const std::string id = vendorId + " " + deviceId + '\0'; // The terminating \0 is important, PDA is not C++
  const char* ids[2] = { id.data(), nullptr };

  DeviceOperator* deviceOperator = DeviceOperator_new(ids, PDA_ENUMERATE_DEVICES);
  if(deviceOperator == NULL){
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get device operator");
  }

  uint64_t deviceCount;
  DeviceOperator_getPciDeviceCount(deviceOperator, &deviceCount);

  for (uint64_t i = 0; i < deviceCount; ++i) {
    PciDevice* pciDevice;
    if (DeviceOperator_getPciDevice(deviceOperator, &pciDevice, 0) != PDA_SUCCESS) {
      ALICEO2_RORC_THROW_EXCEPTION("Failed to get PciDevice");
    }
    pciDevices.push_back(pciDevice);
  }
}

PdaDevice::~PdaDevice()
{
  if (deviceOperator != nullptr) {
    if (DeviceOperator_delete(deviceOperator, PDA_DELETE) != PDA_SUCCESS) {
      ALICEO2_RORC_THROW_EXCEPTION("Failed to delete DeviceOperator");
    }
  }
}

DeviceOperator* PdaDevice::getDeviceOperator()
{
  return deviceOperator;
}

PciDevice* PdaDevice::getPciDevice(int index)
{
  PciDevice* pd;
  if (DeviceOperator_getPciDevice(deviceOperator, &pd, index) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get PciDevice");
  }
  return pd;
}

int PdaDevice::getPciDeviceCount()
{
  uint64_t deviceCount;
  if (DeviceOperator_getPciDeviceCount(deviceOperator, &deviceCount) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get PCI device count");
  }
  return deviceCount;
}

} // namespace Rorc
} // namespace AliceO2

