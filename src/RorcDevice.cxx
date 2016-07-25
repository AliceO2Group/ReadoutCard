///
/// \file RorcDevice.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
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
    const PciId pciId;
    std::function<int(PciDevice* pciDevice)> getSerial;
};

const std::vector<DeviceType> deviceTypes = {
    { CardType::CRORC, {"0033", "10dc"}, crorcGetSerial }, // C-RORC
    { CardType::CRU, {"e001", "1172"}, cruGetSerial }, // Altera dev board CRU
    //{ CardType::CRU, {"????", "10dc"}, cruGetSerial }, // Actual CRU? To be determined...
};

RorcDevice::RorcDevice(int serialNumber)
    : pciId({"unknown", "unknown"}), serialNumber(-1), cardType(CardType::UNKNOWN)
{
  try {
    for (auto& type : deviceTypes) {
      pdaDevice.reset(new PdaDevice(type.pciId));
      auto pciDevices = pdaDevice->getPciDevices();

      for (auto& pciDevice : pciDevices) {
        int serial = type.getSerial(pciDevice);
        if (serial == serialNumber) {
          this->pciDevice = pciDevice;
          this->cardType = type.cardType;
          this->serialNumber = serial;
          this->pciId = type.pciId;
          return;
        }
      }
    }
    BOOST_THROW_EXCEPTION(RorcException() << errinfo_rorc_generic_message("Could not find card"));
  }
  catch (boost::exception& e) {
    e << errinfo_rorc_serial_number(serialNumber);
    addPossibleCauses(e, {"Invalid serial number search target"});
    throw;
  }
}

RorcDevice::~RorcDevice()
{
}

std::vector<RorcDevice::CardDescriptor> RorcDevice::enumerateDevices()
{
  std::vector<RorcDevice::CardDescriptor> cards;

  for (auto& type : deviceTypes) {
    PdaDevice pdaDevice(type.pciId);
    auto pciDevices = pdaDevice.getPciDevices();

    for (auto& pciDevice : pciDevices) {
      RorcDevice::CardDescriptor cd;
      cd.cardType = type.cardType;
      cd.serialNumber = type.getSerial(pciDevice);
      cd.pciId = type.pciId;
      cards.push_back(cd);
    }
  }
  return cards;
}

std::vector<RorcDevice::CardDescriptor> RorcDevice::enumerateDevices(int serialNumber)
{
  std::vector<RorcDevice::CardDescriptor> cards;
  try {
    for (auto& type : deviceTypes) {
      PdaDevice pdaDevice(type.pciId);
      auto pciDevices = pdaDevice.getPciDevices();

      for (auto& pciDevice : pciDevices) {
        int serial = type.getSerial(pciDevice);
        if (serial == serialNumber) {
          RorcDevice::CardDescriptor cd;
          cd.cardType = type.cardType;
          cd.serialNumber = serial;
          cd.pciId = type.pciId;
          cards.push_back(cd);
        }
      }
    }
  }
  catch (boost::exception& e) {
    e << errinfo_rorc_serial_number(serialNumber);
    addPossibleCauses(e, {"Invalid serial number search target"});
    throw;
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
  printf("WARNING: CRU NOT REALLY SUPPORTED, SERIAL NR HARDCODED\n");
  return 12345;
  //BOOST_THROW_EXCEPTION(DeviceFinderException() << errinfo_rorc_generic_message("CRU unsupported"));
}

} // namespace Rorc
} // namespace AliceO2
