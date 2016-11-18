/// \file RorcDevice.cxx
/// \brief Implementation of the RorcDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RorcDevice.h"

#include <map>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/format.hpp>
#include <functional>
#include <iostream>
#include "Crorc/Crorc.h"
#include "Cru/CruBarAccessor.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDevice.h"
#include "RORC/Exception.h"
#include "Util.h"

namespace AliceO2 {
namespace Rorc {

int32_t crorcGetSerial(PciDevice* pciDevice);
int32_t cruGetSerial(PciDevice* pciDevice);

namespace {
struct DeviceType
{
    CardType::type cardType;
    const PciId pciId;
    std::function<int32_t (PciDevice* pciDevice)> getSerial;
};

const std::vector<DeviceType> deviceTypes = {
    { CardType::Crorc, {"0033", "10dc"}, crorcGetSerial }, // C-RORC
    { CardType::Cru, {"e001", "1172"}, cruGetSerial }, // Altera dev board CRU
};

PciAddress addressFromDevice(PciDevice* pciDevice){
  uint8_t busId;
  uint8_t deviceId;
  uint8_t functionId;
  PciDevice_getBusID(pciDevice, &busId);
  PciDevice_getDeviceID(pciDevice, &deviceId);
  PciDevice_getFunctionID(pciDevice, &functionId);
  PciAddress address (busId, deviceId, functionId);
  return address;
}

RorcDevice::CardDescriptor defaultDescriptor() {
  return {CardType::Unknown, -1, {"unknown", "unknown"}, PciAddress(0,0,0)};
}
} // Anonymous namespace

void RorcDevice::initWithSerial(int serialNumber)
{
  try {
    for (const auto& type : deviceTypes) {
      Util::resetSmartPtr(mPdaDevice, type.pciId);
      auto pciDevices = mPdaDevice->getPciDevices();
      for (const auto& pciDevice : pciDevices) {
        int serial = type.getSerial(pciDevice);
        if (serial == serialNumber) {
          mPciDevice = pciDevice;
          mDescriptor = CardDescriptor{type.cardType, serial, type.pciId, addressFromDevice(pciDevice)};
          return;
        }
      }
    }
    BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message("Could not find card"));
  }
  catch (boost::exception& e) {
    e << errinfo_rorc_serial_number(serialNumber);
    addPossibleCauses(e, {"Invalid serial number search target"});
    throw;
  }
}

void RorcDevice::initWithAddress(const PciAddress& address)
{
  try {
    for (const auto& type : deviceTypes) {
      Util::resetSmartPtr(mPdaDevice, type.pciId);
      auto pciDevices = mPdaDevice->getPciDevices();
      for (const auto& pciDevice : pciDevices) {
        auto deviceAddress = addressFromDevice(pciDevice);
        if (deviceAddress == address) {
          mPciDevice = pciDevice;
          mDescriptor = CardDescriptor { type.cardType, type.getSerial(pciDevice), type.pciId, address };
          return;
        }
      }
    }
    BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message("Could not find card"));
  } catch (boost::exception& e) {
    e << errinfo_rorc_pci_address(address);
    addPossibleCauses(e, { "Invalid PCI address search target" });
    throw;
  }
}

RorcDevice::RorcDevice(int serialNumber)
    : mDescriptor(defaultDescriptor())
{
  initWithSerial(serialNumber);
}

RorcDevice::RorcDevice(const PciAddress& address)
    : mDescriptor(defaultDescriptor())
{
  initWithAddress(address);
}

RorcDevice::RorcDevice(const Parameters::CardId::value_type& cardId)
    : mDescriptor(defaultDescriptor())
{
  if (auto serial = boost::get<int>(&cardId)) {
    initWithSerial(*serial);
  } else {
    initWithAddress(boost::get<PciAddress>(cardId));
  }
}

RorcDevice::~RorcDevice()
{
}

std::vector<RorcDevice::CardDescriptor> RorcDevice::findSystemDevices()
{
  std::vector<RorcDevice::CardDescriptor> cards;
  for (const auto& type : deviceTypes) {
    Pda::PdaDevice pdaDevice(type.pciId);
    auto pciDevices = pdaDevice.getPciDevices();
    for (const auto& pciDevice : pciDevices) {
      cards.push_back(CardDescriptor{type.cardType, type.getSerial(pciDevice), type.pciId,
        addressFromDevice(pciDevice)});
    }
  }
  return cards;
}

std::vector<RorcDevice::CardDescriptor> RorcDevice::findSystemDevices(int serialNumber)
{
  std::vector<RorcDevice::CardDescriptor> cards;
  try {
    for (const auto& type : deviceTypes) {
      Pda::PdaDevice pdaDevice(type.pciId);
      auto pciDevices = pdaDevice.getPciDevices();
      for (const auto& pciDevice : pciDevices) {
        int serial = type.getSerial(pciDevice);
        if (serial == serialNumber) {
          cards.push_back(CardDescriptor{type.cardType, type.getSerial(pciDevice), type.pciId,
                  addressFromDevice(pciDevice)});
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

std::vector<RorcDevice::CardDescriptor> RorcDevice::findSystemDevices(const PciAddress& address)
{
  std::vector<RorcDevice::CardDescriptor> cards;
  try {
    for (const auto& type : deviceTypes) {
      Pda::PdaDevice pdaDevice(type.pciId);
      auto pciDevices = pdaDevice.getPciDevices();
      for (const auto& pciDevice : pciDevices) {
        auto deviceAddress = addressFromDevice(pciDevice);

        printf("Address: %s\n", deviceAddress.toString().c_str());

        if (deviceAddress == address) {
          cards.push_back(CardDescriptor{type.cardType, type.getSerial(pciDevice), type.pciId, address});
        }
      }
    }
  }
  catch (boost::exception& e) {
    e << errinfo_rorc_pci_address(address);
    addPossibleCauses(e, {"Invalid PCI address search target"});
    throw;
  }
  return cards;
}

void RorcDevice::printDeviceInfo(std::ostream& ostream)
{
  uint16_t domainId;
  PciDevice_getDomainID(mPciDevice, &domainId);

  uint8_t busId;
  PciDevice_getBusID(mPciDevice, &busId);

  uint8_t functionId;
  PciDevice_getFunctionID(mPciDevice, &functionId);

  const PciBarTypes* pciBarTypesPtr;
  PciDevice_getBarTypes(mPciDevice, &pciBarTypesPtr);
  auto barType = *pciBarTypesPtr;
  auto barTypeString =
      barType == PCIBARTYPES_NOT_MAPPED ? "NOT_MAPPED" :
      barType == PCIBARTYPES_IO ? "IO" :
      barType == PCIBARTYPES_BAR32 ? "BAR32" :
      barType == PCIBARTYPES_BAR64 ? "BAR64" :
      "n/a";

//  auto f = boost::format("%-14s %10s\n");
//  ostream << f % "Domain ID" << domainId;
//  ostream << f % "Bus ID" << busId;
//  ostream << f % "Function ID" << functionId;
//  ostream << f % "BAR type" << barTypeString;
}

int32_t cruGetSerial(PciDevice* pciDevice)
{
  int channel = 2; // Must use BAR 2 to access serial number
  Pda::PdaBar pdaBar(pciDevice, channel);
  return CruBarAccessor(&pdaBar).getSerialNumber();
}

// The RORC headers have a lot of macros that cause problems with the rest of this file, so we include it down here.
#include "c/rorc/rorc.h"

int32_t crorcGetSerial(PciDevice* pciDevice)
{
  int channel = 0; // Must use BAR 0 to access flash
  Pda::PdaBar pdaBar(pciDevice, channel);
  uint32_t serial = Crorc::getSerial(pdaBar.getUserspaceAddress());
  return serial;
}

} // namespace Rorc
} // namespace AliceO2
