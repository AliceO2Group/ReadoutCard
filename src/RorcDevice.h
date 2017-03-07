/// \file RorcDevice.h
/// \brief Definition of the RorcDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include <string>
#include "CardDescriptor.h"
#include "Pda/PdaDevice.h"
#include "RORC/CardType.h"
#include "RORC/Parameters.h"
#include "RORC/PciAddress.h"
#include "RORC/PciId.h"

namespace AliceO2 {
namespace Rorc {

/// Represents a single RORC PCI device
class RorcDevice
{
  public:
    RorcDevice(int serialNumber);

    RorcDevice(const PciAddress& address);

    RorcDevice(const Parameters::CardIdType& cardId);

    ~RorcDevice();

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

    // Finds RORC devices on the system
    static std::vector<CardDescriptor> findSystemDevices();

    // Finds RORC devices on the system with the given serial number
    static std::vector<CardDescriptor> findSystemDevices(int serialNumber);

    // Finds RORC devices on the system with the given address
    static std::vector<CardDescriptor> findSystemDevices(const PciAddress& address);

  private:

    void initWithSerial(int serialNumber);
    void initWithAddress(const PciAddress& address);

    Pda::PdaDevice::SharedPdaDevice mPdaDevice;
    std::unique_ptr<Pda::PdaDevice::PdaPciDevice> mPciDevice;
    CardDescriptor mDescriptor;
};

} // namespace Rorc
} // namespace AliceO2
