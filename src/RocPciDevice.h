/// \file RocPciDevice.h
/// \brief Definition of the RocPciDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include <string>
#include "CardDescriptor.h"
#include "Pda/PdaDevice.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/PciId.h"

namespace AliceO2 {
namespace roc {

/// Represents a single ReadoutCard PCI device
class RocPciDevice
{
  public:
    RocPciDevice(int serialNumber);

    RocPciDevice(const PciAddress& address);

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

    int getSerialNumber() const
    {
      return mDescriptor.serialNumber;
    }

    PciAddress getPciAddress() const
    {
      return mDescriptor.pciAddress;
    }

    Pda::PdaDevice::PdaPciDevice getPciDevice() const
    {
      return *mPciDevice.get();
    }

    void printDeviceInfo(std::ostream& ostream);

    // Finds ReadoutCard devices on the system
    static std::vector<CardDescriptor> findSystemDevices();

    // Finds ReadoutCard devices on the system with the given serial number
    static std::vector<CardDescriptor> findSystemDevices(int serialNumber);

    // Finds ReadoutCard devices on the system with the given address
    static std::vector<CardDescriptor> findSystemDevices(const PciAddress& address);

  private:

    void initWithSerial(int serialNumber);
    void initWithAddress(const PciAddress& address);

    Pda::PdaDevice::SharedPdaDevice mPdaDevice;
    std::unique_ptr<Pda::PdaDevice::PdaPciDevice> mPciDevice;
    CardDescriptor mDescriptor;
};

} // namespace roc
} // namespace AliceO2
