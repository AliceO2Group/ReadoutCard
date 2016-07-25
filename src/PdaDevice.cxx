///
/// \file PdaDevice.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "PdaDevice.h"
#include <iostream>
#include <map>
#include <pda.h>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include "RorcException.h"

/// Throws the given exception if the given status code is not equal to RORC_STATUS_OK
#define THROW_IF_BAD_STATUS(_status_code_in, _exception) do { \
  auto _status_code = _status_code_in; \
  if (_status_code != PDA_SUCCESS) { \
    BOOST_THROW_EXCEPTION(_exception << errinfo_rorc_pda_status_code(_status_code)); \
  } \
} while(0)

namespace AliceO2 {
namespace Rorc {

namespace b = boost;
namespace bfs = boost::filesystem;

PdaDevice::PdaDevice(const PciId& pciId) : deviceOperator(nullptr)
{
  try {
    THROW_IF_BAD_STATUS(PDAInit(), RorcPdaException() << errinfo_rorc_generic_message("Failed to initialize PDA"));

    // The terminating \0 is important, PDA is not C++
    const std::string id = pciId.getVendorId() + " " + pciId.getDeviceId() + '\0';
    const char* ids[2] = { id.data(), nullptr };

    DeviceOperator* deviceOperator = DeviceOperator_new(ids, PDA_ENUMERATE_DEVICES);
    if(deviceOperator == NULL){
      BOOST_THROW_EXCEPTION(RorcPdaException()
          << errinfo_rorc_generic_message("Failed to get DeviceOperator")
          << errinfo_rorc_possible_causes({"Invalid PCI ID"}));
    }

    uint64_t deviceCount = getPciDeviceCount();

    for (uint64_t i = 0; i < deviceCount; ++i) {
      PciDevice* pciDevice = getPciDevice(i);
      pciDevices.push_back(pciDevice);
    }
  }
  catch (boost::exception& e) {
    e << errinfo_rorc_pci_id(pciId);
    addPossibleCauses(e, {"Driver module not inserted (> modprobe uio_pci_dma)"});
    throw;
  }
}

PdaDevice::~PdaDevice()
{
  if (deviceOperator != nullptr) {
    if (DeviceOperator_delete(deviceOperator, PDA_DELETE) != PDA_SUCCESS) {
      std::cerr << "Failed to delete DeviceOperator" << std::endl;
    }
  }
}

DeviceOperator* PdaDevice::getDeviceOperator()
{
  return deviceOperator;
}

PciDevice* PdaDevice::getPciDevice(int index)
{
  PciDevice* pciDevice;
  THROW_IF_BAD_STATUS(DeviceOperator_getPciDevice(deviceOperator, &pciDevice, index), RorcPdaException()
      << errinfo_rorc_generic_message("Failed to get PciDevice")
      << errinfo_rorc_pci_device_index(index));
  return pciDevice;
}

int PdaDevice::getPciDeviceCount()
{
  uint64_t deviceCount;
  THROW_IF_BAD_STATUS(DeviceOperator_getPciDeviceCount(deviceOperator, &deviceCount), RorcPdaException()
      << errinfo_rorc_generic_message("Failed to get PCI device count"));
  return deviceCount;
}

} // namespace Rorc
} // namespace AliceO2

