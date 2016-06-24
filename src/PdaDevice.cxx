///
/// \file PdaDevice.cxx
/// \author Pascal Boeschoten
///

#include "PdaDevice.h"
#include <pda.h>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

namespace b = boost;
namespace bfs = boost::filesystem;

std::string PdaDevice::fileToString(const bfs::path& path)
{
  bfs::ifstream fs(path);
  std::stringstream buffer;
  buffer << fs.rdbuf();
  return buffer.str();
}

PdaDevice::PdaDevice(int serialNumber) : deviceOperator(nullptr), pciDevice(nullptr)
{
  if (PDAInit() != PDA_SUCCESS) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to initialize PDA");
  }

  const std::string CRORC_DEVICE_ID = "0033";
  const std::string CERN_VENDOR_ID = "10dc";
  const bfs::path dirPath("/sys/bus/pci/devices/");
  const bfs::path vendorFile("vendor");
  const bfs::path deviceFile("device");

  if (!bfs::exists(dirPath)) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to open directory");
  }

  for (auto& entry : boost::make_iterator_range(bfs::directory_iterator(dirPath), bfs::directory_iterator())) {
    const bfs::path dir = entry.path();

    // Get vendor & device ID, trimming off the leading '0x'
    const std::string vendorId = fileToString(dir/"vendor").substr(2, 4);
    const std::string deviceId = fileToString(dir/"device").substr(2, 4);

    if (vendorId != CERN_VENDOR_ID) {
      // Not a CERN card
      continue;
    }

    if (deviceId == CRORC_DEVICE_ID) {
      newDeviceOperator(vendorId, deviceId);
      getPciDevice(serialNumber);
      return;
    }
  }
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

void PdaDevice::newDeviceOperator(const std::string& vendorId, const std::string& deviceId)
{
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
}

void PdaDevice::getPciDevice(int serialNumber)
{
  if (pciDevice != nullptr) {
    ALICEO2_RORC_THROW_EXCEPTION("PciDevice was already initialized");
  }

  // Getting the card.
  PciDevice* pd;
  if(DeviceOperator_getPciDevice(deviceOperator, &pd, serialNumber) != PDA_SUCCESS){
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get PciDevice");
  }

  pciDevice = pd;
}

} // namespace Rorc
} // namespace AliceO2
