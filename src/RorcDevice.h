/// \file RorcDevice.h
/// \brief Definition of the RorcDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <string>
#include <boost/scoped_ptr.hpp>
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

    const PciId& getPciId() const
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

    PciDevice* getPciDevice() const
    {
      return mPciDevice;
    }

    void printDeviceInfo(std::ostream& ostream);

    struct CardDescriptor
    {
        CardType::type cardType;
        int serialNumber;
        PciId pciId;
        PciAddress pciAddress;
    };

    // Finds RORC devices on the system
    static std::vector<CardDescriptor> findSystemDevices();

    // Finds RORC devices on the system with the given serial number
    static std::vector<CardDescriptor> findSystemDevices(int serialNumber);

    // Finds RORC devices on the system with the given address
    static std::vector<CardDescriptor> findSystemDevices(const PciAddress& address);

  private:

    void initWithSerial(int serialNumber);
    void initWithAddress(const PciAddress& address);

    boost::scoped_ptr<Pda::PdaDevice> mPdaDevice;
    PciDevice* mPciDevice;
    CardDescriptor mDescriptor;
};

} // namespace Rorc
} // namespace AliceO2
