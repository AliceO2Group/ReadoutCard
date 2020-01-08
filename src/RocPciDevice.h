// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file RocPciDevice.h
/// \brief Definition of the RocPciDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_ROCPCIDEVICE_H_
#define ALICEO2_SRC_READOUTCARD_ROCPCIDEVICE_H_

#include <memory>
#include <string>
#include "Pda/PdaBar.h"
#include "Pda/PdaDevice.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/CardDescriptor.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/PciId.h"

namespace AliceO2
{
namespace roc
{

/// Represents a single ReadoutCard PCI device
class RocPciDevice
{
 public:
  RocPciDevice(const Parameters::CardIdType& cardId);

  ~RocPciDevice();

  CardDescriptor getCardDescriptor() const
  {
    return mDescriptor;
  }

  PciId getPciId() const
  {
    return mDescriptor.pciId;
  }

  CardType::type getCardType() const
  {
    return mDescriptor.cardType;
  }

  PciAddress getPciAddress() const
  {
    return mDescriptor.pciAddress;
  }

  PciDevice* getPciDevice() const
  {
    return mPciDevice;
  }

  int getSerialId() const //TODO: Could be used for logging?
  {
    return mDescriptor.serialId.getSerial();
  }

  std::shared_ptr<Pda::PdaBar> getBar(int barIndex)
  {
    if (barIndex == 0) {
      return std::move(mPdaBar0);
    } else if (barIndex == 2) {
      return std::move(mPdaBar2);
    }
    return nullptr;
  }

  void printDeviceInfo(std::ostream& ostream);

  // Finds ReadoutCard devices on the system
  static std::vector<CardDescriptor> findSystemDevices();

  // Finds ReadoutCard devices on the system with the given card id //TODO: Used for what?
  //static std::vector<CardDescriptor> findSystemDevices(const Parameters::CardIdType& cardId);

 private:
  void initWithSerialId(const SerialId& serialId);
  void initWithAddress(const PciAddress& address);
  void initWithSequenceNumber(const PciSequenceNumber& sequenceNumber);

  PciDevice* mPciDevice;
  CardDescriptor mDescriptor;
  std::shared_ptr<Pda::PdaBar> mPdaBar0;
  std::shared_ptr<Pda::PdaBar> mPdaBar2;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_ROCPCIDEVICE_H_
