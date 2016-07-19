///
/// \file RorcDevice.cxx
/// \author Pascal Boeschoten
///

#include "RorcDevice.h"

#include <map>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include <functional>
#include <iostream>
#include "PdaBar.h"
#include "PdaDevice.h"
#include "RorcException.h"
#include "Crorc.h"

namespace AliceO2 {
namespace Rorc {

int crorcGetSerial(PciDevice* pciDevice);
int cruGetSerial(PciDevice* pciDevice);

struct DeviceDescriptor
{
    CardType::type cardType;
    const std::string vendorId;
    const std::string deviceId;
    std::function<int(PciDevice* pciDevice)> getSerial;
};

const std::vector<DeviceDescriptor> deviceDescriptors = {
    { CardType::CRORC, "10dc", "0033", crorcGetSerial },
    { CardType::CRU, "10dc", "????", cruGetSerial },
};

RorcDevice::RorcDevice(int serialNumber)
    : deviceId("unknown"), vendorId("unknown"), serialNumber(-1), cardType(CardType::UNKNOWN)
{
  for (auto& dd : deviceDescriptors) {
    pdaDevice.reset(new PdaDevice(dd.vendorId, dd.deviceId));
    auto pciDevices = pdaDevice->getPciDevices();

    for (auto& pciDevice : pciDevices) {
      int serial = dd.getSerial(pciDevice);
      printf("found serial: %d\n");
      if (serial == serialNumber) {
        this->pciDevice = pciDevice;
        this->cardType = dd.cardType;
        this->serialNumber = serial;
        this->deviceId = dd.deviceId;
        this->vendorId = dd.vendorId;
        return;
      }
    }
  }

  BOOST_THROW_EXCEPTION(RorcException()
      << errinfo_rorc_generic_message("Could not find card")
      << errinfo_rorc_serial_number(serialNumber));
}

RorcDevice::~RorcDevice()
{
}

// The RORC headers have a lot of macros that cause problems with the rest of this file, so we include it down here.
#include "c/rorc/rorc.h"

// TODO Clean up, C++ificate
int crorcGetSerial(PciDevice* pciDevice)
{
  int channel = 0; // Must use channel 0 to access flash
  PdaBar pdaBar(pciDevice, channel);
  return Crorc::getSerial(pdaBar.getUserspaceAddress());
}

int cruGetSerial(PciDevice* pciDevice)
{
  BOOST_THROW_EXCEPTION(DeviceFinderException() << errinfo_rorc_generic_message("CRU unsupported"));
}


} // namespace Rorc
} // namespace AliceO2
