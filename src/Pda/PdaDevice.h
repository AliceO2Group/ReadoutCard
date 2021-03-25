// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file PdaDevice.h
/// \brief Definition of the PdaDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_SRC_PDA_PDADEVICE_H_
#define O2_READOUTCARD_SRC_PDA_PDADEVICE_H_

#include <memory>
#include <string>
#include <vector>
#include <pda.h>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/PciId.h"

namespace o2
{
namespace roc
{
namespace Pda
{

struct PciType {
  CardType::type cardType;
  const PciId pciId;
};

struct TypedPciDevice {
  CardType::type cardType;
  PciDevice* pciDevice;
};

/// Handles the creation and cleanup of the PDA DeviceOperator and PciDevice objects
class PdaDevice
{
 public:
  static PdaDevice& Instance()
  {
    static PdaDevice theInstance;
    return theInstance;
  }

  static std::vector<TypedPciDevice> getPciDevices()
  {
    auto pdaDevice = &PdaDevice::Instance();
    return pdaDevice->mPciDevices;
  }

  PdaDevice(PdaDevice const&) = delete;            // Copy construct
  PdaDevice(PdaDevice&&) = delete;                 // Move construct
  PdaDevice& operator=(PdaDevice const&) = delete; // Copy assign
  PdaDevice& operator=(PdaDevice&&) = delete;      // Move assign

 protected:
  PdaDevice();
  ~PdaDevice();

 private:
  int getPciDeviceCount();
  PciDevice* getPciDevice(int index);

  DeviceOperator* mDeviceOperator;
  std::vector<TypedPciDevice> mPciDevices;
};

} // namespace Pda
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_PDA_PDADEVICE_H_
