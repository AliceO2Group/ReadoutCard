
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
/// \file RocPciDevice.h
/// \brief Definition of the RocPciDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_ROCPCIDEVICE_H_
#define O2_READOUTCARD_SRC_ROCPCIDEVICE_H_

#include <memory>
#include <string>
#include "Pda/PdaBar.h"
#include "Pda/PdaDevice.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/CardDescriptor.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/ParameterTypes/SerialId.h"
#include "ReadoutCard/PciId.h"
#include "Utilities/SmartPointer.h"

namespace o2
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

  SerialId getSerialId() const
  {
    return mDescriptor.serialId;
  }

  int getSequenceId() const
  {
    return mDescriptor.sequenceId;
  }

  std::shared_ptr<Pda::PdaBar> getBar(int barIndex)
  {
    if (barIndex == 0) {
      return std::move(mPdaBar0);
    } else if (barIndex == 2) {
      return std::move(mPdaBar2);
    } else if (barIndex > 0 && barIndex < 6) {
      std::shared_ptr<Pda::PdaBar> mPdaBarX;
      Utilities::resetSmartPtr(mPdaBarX, mPciDevice, barIndex);
      return mPdaBarX;
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
} // namespace o2

#endif // O2_READOUTCARD_SRC_ROCPCIDEVICE_H_
