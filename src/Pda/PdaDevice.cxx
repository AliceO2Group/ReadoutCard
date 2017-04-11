/// \file PdaDevice.cxx
/// \brief Implementation of the PdaDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "PdaDevice.h"
#include <iostream>
#include <map>
#include <pda.h>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include "ExceptionInternal.h"

/// Throws the given exception if the given status code is not equal to RORC_STATUS_OK
#define THROW_IF_BAD_STATUS(_status_code_in, _exception) do { \
  auto _status_code = _status_code_in; \
  if (_status_code != PDA_SUCCESS) { \
    BOOST_THROW_EXCEPTION(_exception << ErrorInfo::PdaStatusCode(_status_code)); \
  } \
} while(0)

namespace AliceO2 {
namespace Rorc {
namespace Pda {

namespace b = boost;
namespace bfs = boost::filesystem;

PdaDevice::PdaDevice(const PciId& pciId) : mDeviceOperator(nullptr)
{
  try {
    THROW_IF_BAD_STATUS(PDAInit(), RorcPdaException() << ErrorInfo::Message("Failed to initialize PDA"));

    // The terminating \0 is important, PDA is not C++
    const std::string id = pciId.getVendorId() + " " + pciId.getDeviceId() + '\0';
    const char* ids[2] = { id.data(), nullptr };

    mDeviceOperator = DeviceOperator_new(ids, PDA_ENUMERATE_DEVICES);
    if(mDeviceOperator == nullptr){
      BOOST_THROW_EXCEPTION(RorcPdaException()
          << ErrorInfo::Message("Failed to get DeviceOperator")
          << ErrorInfo::PossibleCauses({"Invalid PCI ID",
              "Insufficient permissions (must be root or member of group 'pda')"}));
    }

    uint64_t deviceCount = getPciDeviceCount();

    for (uint64_t i = 0; i < deviceCount; ++i) {
      PciDevice* pciDevice = getPciDevice(i);
      mPciDevices.push_back(pciDevice);
    }
  }
  catch (boost::exception& e) {
    e << ErrorInfo::PciId(pciId);
    addPossibleCauses(e, {"Driver module not inserted (> modprobe uio_pci_dma)",
        "Driver module doesn't match kernel version"});
    throw;
  }
}

PdaDevice::~PdaDevice()
{
  if (mDeviceOperator != nullptr) {
    if (DeviceOperator_delete(mDeviceOperator, PDA_DELETE) != PDA_SUCCESS) {
      std::cerr << "Failed to delete DeviceOperator; "
          "An associated DMA buffer's memory may have been unmapped by the user" << std::endl;
    }
  }
}

PciDevice* PdaDevice::getPciDevice(int index)
{
  PciDevice* pciDevice;
  THROW_IF_BAD_STATUS(DeviceOperator_getPciDevice(mDeviceOperator, &pciDevice, index), RorcPdaException()
      << ErrorInfo::Message("Failed to get PciDevice")
      << ErrorInfo::PciDeviceIndex(index));
  return pciDevice;
}

int PdaDevice::getPciDeviceCount()
{
  uint64_t deviceCount;
  THROW_IF_BAD_STATUS(DeviceOperator_getPciDeviceCount(mDeviceOperator, &deviceCount), RorcPdaException()
      << ErrorInfo::Message("Failed to get PCI device count"));
  return deviceCount;
}

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2

