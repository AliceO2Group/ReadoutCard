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

PdaDevice::PdaDevice(std::string vendorId, std::string deviceId) : deviceOperator(nullptr), pciDevice(nullptr)
{
  if (PDAInit() != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to initialize PDA");
  }

  if (deviceOperator != nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("DeviceOperator was already initialized");
  }

  const std::string id = vendorId + " " + deviceId + '\0'; // The terminating \0 is important, PDA is not C++
  const char* ids[2] = { id.data(), nullptr };

  DeviceOperator* dop = DeviceOperator_new(ids, PDA_ENUMERATE_DEVICES);
  if(dop == NULL){
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get device operator");
  }

  deviceOperator = dop;

  if (pciDevice != nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("PciDevice was already initialized");
  }

  // Getting the card.
  PciDevice* pd;
  if (DeviceOperator_getPciDevice(deviceOperator, &pd, 0) != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get PciDevice");
  }

  pciDevice = pd;
}

PdaDevice::~PdaDevice()
{
  if (deviceOperator != nullptr) {
    if(DeviceOperator_delete(deviceOperator, PDA_DELETE) != PDA_SUCCESS) {
      ALICEO2_RORC_THROW_EXCEPTION("Failed to delete DeviceOperator");
    }
  }
}

DeviceOperator* PdaDevice::getDeviceOperator()
{
  return deviceOperator;
}

PciDevice* PdaDevice::getPciDevice()
{
  return pciDevice;
}


} // namespace Rorc
} // namespace AliceO2
