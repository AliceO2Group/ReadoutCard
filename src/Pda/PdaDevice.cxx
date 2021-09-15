
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file PdaDevice.cxx
/// \brief Implementation of the PdaDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "PdaDevice.h"
#include <iostream>
#include <map>
#include <pda.h>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include "ExceptionInternal.h"

/// Throws the given exception if the given status code is not equal to PDA_SUCCESS
#define THROW_IF_BAD_STATUS(_status_code_in, _exception)                           \
  do {                                                                             \
    auto _status_code = _status_code_in;                                           \
    if (_status_code != PDA_SUCCESS) {                                             \
      BOOST_THROW_EXCEPTION(_exception << ErrorInfo::PdaStatusCode(_status_code)); \
    }                                                                              \
  } while (0)

namespace o2
{
namespace roc
{
namespace Pda
{

namespace b = boost;
namespace bfs = boost::filesystem;

PdaDevice::PdaDevice() : mDeviceOperator(nullptr)
{
  try {
    THROW_IF_BAD_STATUS(PDAInit(), PdaException() << ErrorInfo::Message("Failed to initialize PDA"));

    const std::vector<PciType> pciTypes = {
      { CardType::Crorc, { "0033", "10dc" } },
      { CardType::Cru, { "0034", "10dc" } },
      { CardType::Cru, { "e001", "1172" } } // Altera vendor & device id; to be discontinued
    };

    for (const PciType& pciType : pciTypes) {
      const PciId pciId = pciType.pciId;

      // The terminating \0 is important, PDA is not C++
      const std::string id = pciId.getVendorId() + " " + pciId.getDeviceId() + '\0';
      const char* ids[2] = { id.data(), nullptr };

      mDeviceOperator = DeviceOperator_new(ids, PDA_DONT_ENUMERATE_DEVICES);
      if (mDeviceOperator == nullptr) {
        BOOST_THROW_EXCEPTION(PdaException()
                              << ErrorInfo::Message("Failed to get DeviceOperator")
                              << ErrorInfo::PossibleCauses({ "Invalid PCI ID",
                                                             "Insufficient permissions (must be root or member of group 'pda')" }));
      }

      uint64_t deviceCount = getPciDeviceCount();

      for (uint64_t i = 0; i < deviceCount; ++i) {
        PciDevice* pciDevice = getPciDevice(i);
        mPciDevices.push_back({ pciType.cardType, pciDevice });
      }
    }
  } catch (boost::exception& e) {
    addPossibleCauses(e, { "Driver module not inserted (> modprobe uio_pci_dma)",
                           "PDA kernel module version doesn't match kernel version",
                           "PDA userspace library version incompatible with PDA kernel module version (> modinfo uio_pci_dma)" });
    throw;
  }
}

PdaDevice::~PdaDevice()
{
  if (mDeviceOperator != nullptr) {
    if (DeviceOperator_delete(mDeviceOperator, PDA_DELETE) != PDA_SUCCESS) {
      std::cerr << "Failed to delete DeviceOperator; "
                   "An associated DMA buffer's memory may have been unmapped by the user"
                << std::endl;
    }
  }
}

PciDevice* PdaDevice::getPciDevice(int index)
{
  PciDevice* pciDevice;
  THROW_IF_BAD_STATUS(DeviceOperator_getPciDevice(mDeviceOperator, &pciDevice, index), PdaException()
                                                                                         << ErrorInfo::Message("Failed to get PciDevice")
                                                                                         << ErrorInfo::PciDeviceIndex(index));
  return pciDevice;
}

int PdaDevice::getPciDeviceCount()
{
  uint64_t deviceCount;
  THROW_IF_BAD_STATUS(DeviceOperator_getPciDeviceCount(mDeviceOperator, &deviceCount), PdaException()
                                                                                         << ErrorInfo::Message("Failed to get PCI device count"));
  return deviceCount;
}

} // namespace Pda
} // namespace roc
} // namespace o2
