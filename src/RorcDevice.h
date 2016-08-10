///
/// \file RorcDevice.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <string>
#include <boost/scoped_ptr.hpp>
#include "Pda/PdaDevice.h"
#include "RORC/CardType.h"
#include "RORC/PciId.h"

namespace AliceO2 {
namespace Rorc {

/// Represents a single RORC PCI device
class RorcDevice
{
  public:
    RorcDevice(int serialNumber);
    ~RorcDevice();

    const PciId& getPciId() const
    {
      return pciId;
    }

    CardType::type getCardType() const
    {
      return cardType;
    }

    int getSerialNumber() const
    {
      return serialNumber;
    }

    PciDevice* getPciDevice() const
    {
      return pciDevice;
    }

    struct CardDescriptor
    {
        CardType::type cardType;
        int serialNumber;
        PciId pciId;
    };

    // Finds RORC devices on the system
    static std::vector<CardDescriptor> findSystemDevices();

    // Finds RORC devices on the system with the given serial number
    static std::vector<CardDescriptor> findSystemDevices(int serialNumber);

  private:
    boost::scoped_ptr<Pda::PdaDevice> pdaDevice;
    PciDevice* pciDevice;
    PciId pciId;
    int serialNumber;
    CardType::type cardType;
};

} // namespace Rorc
} // namespace AliceO2
