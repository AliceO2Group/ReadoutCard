///
/// \file RorcDeviceEnumerator.cxx
/// \author Pascal Boeschoten
///

#include "RorcDeviceEnumerator.h"

#include <map>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include <functional>
#include <tuple>
#include <iostream>
#include "PdaBar.h"
#include "PdaDevice.h"
#include "RorcException.h"
#include "c/rorc/rorc.h"
#include "Crorc.h"

namespace AliceO2 {
namespace Rorc {

int crorcGetSerialNumber(PciDevice* pciDevice);
int cruGetSerialNumber(PciDevice* pciDevice);

struct DeviceDescriptor
{
    CardType::type cardType;
    const std::string vendorId;
    const std::string deviceId;
    std::function<int(PciDevice* pciDevice)> getSerial;
};

const std::vector<DeviceDescriptor> deviceDescriptors = {
    { CardType::CRORC, "10dc", "0033", crorcGetSerialNumber },
    { CardType::CRU, "10dc", "????", cruGetSerialNumber },
};

RorcDeviceEnumerator::RorcDeviceEnumerator()
{
  for (auto& dd : deviceDescriptors) {
    PdaDevice pdaDevice(dd.vendorId, dd.deviceId);
    auto pciDevices = pdaDevice.getPciDevices();

    for (auto& pciDevice : pciDevices) {
      int serialNumber = dd.getSerial(pciDevice);
      CardDescriptor cd;
      cd.cardType = dd.cardType;
      cd.serialNumber = serialNumber;
      cd.deviceId = dd.deviceId;
      cd.vendorId = dd.vendorId;
      cardsFound.push_back(cd);
    }
  }
}

RorcDeviceEnumerator::RorcDeviceEnumerator(int serialNumber)
{
  for (auto& dd : deviceDescriptors) {
    PdaDevice pdaDevice(dd.vendorId, dd.deviceId);
    auto pciDevices = pdaDevice.getPciDevices();

    for (auto& pciDevice : pciDevices) {
      int serial = dd.getSerial(pciDevice);
      printf("found serial: %d\n");
        if (serial == serialNumber) {
        CardDescriptor cd;
        cd.cardType = dd.cardType;
        cd.serialNumber = serial;
        cd.deviceId = dd.deviceId;
        cd.vendorId = dd.vendorId;
        cardsFound.push_back(cd);
      }
    }
  }
}

RorcDeviceEnumerator::~RorcDeviceEnumerator()
{
}

int crorcGetSerialNumber(PciDevice* pciDevice)
{
  int channel = 0; // Must use channel 0 to access flash
  PdaBar pdaBar(pciDevice, channel);
  return Crorc::getSerial(pdaBar.getUserspaceAddress());
}

int cruGetSerialNumber(PciDevice* pciDevice)
{
  BOOST_THROW_EXCEPTION(DeviceFinderException() << errinfo_rorc_generic_message("CRU unsupported"));
}


} // namespace Rorc
} // namespace AliceO2
