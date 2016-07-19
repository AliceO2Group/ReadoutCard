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

struct DeviceType
{
    CardType::type cardType;
    const std::string vendorId;
    const std::string deviceId;
    std::function<int(PciDevice* pciDevice)> getSerial;
};

const std::vector<DeviceType> deviceTypes = {
    { CardType::CRORC, "10dc", "0033", crorcGetSerial },
    { CardType::CRU, "10dc", "????", cruGetSerial },
};

RorcDevice::RorcDevice(int serialNumber)
    : deviceId("unknown"), vendorId("unknown"), serialNumber(-1), cardType(CardType::UNKNOWN)
{
  for (auto& type : deviceTypes) {
    pdaDevice.reset(new PdaDevice(type.vendorId, type.deviceId));
    auto pciDevices = pdaDevice->getPciDevices();

    for (auto& pciDevice : pciDevices) {
      int serial = type.getSerial(pciDevice);
      if (serial == serialNumber) {
        this->pciDevice = pciDevice;
        this->cardType = type.cardType;
        this->serialNumber = serial;
        this->deviceId = type.deviceId;
        this->vendorId = type.vendorId;
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

std::vector<RorcDevice::CardDescriptor> RorcDevice::enumerateDevices()
{
  std::vector<RorcDevice::CardDescriptor> cards;

  for (auto& type : deviceTypes) {
    PdaDevice pdaDevice(type.vendorId, type.deviceId);
    auto pciDevices = pdaDevice.getPciDevices();

    for (auto& pciDevice : pciDevices) {
      RorcDevice::CardDescriptor cd;
      cd.cardType = type.cardType;
      cd.serialNumber = type.getSerial(pciDevice);
      cd.deviceId = type.deviceId;
      cd.vendorId = type.vendorId;
      cards.push_back(cd);
    }
  }
  return cards;
}

std::vector<RorcDevice::CardDescriptor> RorcDevice::enumerateDevices(int serialNumber)
{
  std::vector<RorcDevice::CardDescriptor> cards;

  for (auto& type : deviceTypes) {
    PdaDevice pdaDevice(type.vendorId, type.deviceId);
    auto pciDevices = pdaDevice.getPciDevices();

    for (auto& pciDevice : pciDevices) {
      int serial = type.getSerial(pciDevice);
      if (serial == serialNumber) {
        RorcDevice::CardDescriptor cd;
        cd.cardType = type.cardType;
        cd.serialNumber = serial;
        cd.deviceId = type.deviceId;
        cd.vendorId = type.vendorId;
        cards.push_back(cd);
      }
    }
  }
  return cards;
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

int cruGetSerial(PciDevice*)
{
  BOOST_THROW_EXCEPTION(DeviceFinderException() << errinfo_rorc_generic_message("CRU unsupported"));
}


} // namespace Rorc
} // namespace AliceO2
