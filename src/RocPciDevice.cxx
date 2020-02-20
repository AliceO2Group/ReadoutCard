// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file RocPciDevice.cxx
/// \brief Implementation of the RocPciDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "RocPciDevice.h"

#include <map>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <functional>
#include <iostream>
#include "Crorc/Crorc.h"
#include "Cru/CruBar.h"
#include "Pda/PdaDevice.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/Parameters.h"

namespace AliceO2
{
namespace roc
{

int crorcGetSerial(std::shared_ptr<Pda::PdaBar> pdaBar);
int crorcGetEndpoint(std::shared_ptr<Pda::PdaBar> pdaBar);
int cruGetSerial(std::shared_ptr<Pda::PdaBar> pdaBar);
int cruGetEndpoint(std::shared_ptr<Pda::PdaBar> pdaBar);

namespace
{
struct DeviceType {
  CardType::type cardType;
  PciId pciId;
  std::function<int(std::shared_ptr<Pda::PdaBar> pdaBar)> getSerial;
  std::function<int(std::shared_ptr<Pda::PdaBar> pdaBar)> getEndpoint;
};

const std::vector<DeviceType> deviceTypes = {
  { CardType::Crorc, { "0033", "10dc" }, crorcGetSerial, crorcGetEndpoint }, // C-RORC
  { CardType::Cru, { "e001", "1172" }, cruGetSerial, cruGetEndpoint },       // Altera dev board CRU
};

PciAddress addressFromDevice(PciDevice* pciDevice)
{
  uint8_t busId;
  uint8_t deviceId;
  uint8_t functionId;
  if (PciDevice_getBusID(pciDevice, &busId) || PciDevice_getDeviceID(pciDevice, &deviceId) || PciDevice_getFunctionID(pciDevice, &functionId)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Failed to retrieve device address"));
  }
  PciAddress address(busId, deviceId, functionId);
  return address;
}

CardDescriptor defaultDescriptor()
{
  return { CardType::Unknown, SerialId(-1, 0), { "unknown", "unknown" }, PciAddress(0, 0, 0), -1 };
}
} // Anonymous namespace

RocPciDevice::RocPciDevice(const Parameters::CardIdType& cardId)
  : mDescriptor(defaultDescriptor())
{
  if (auto serialId = boost::get<SerialId>(&cardId)) {
    initWithSerialId(*serialId);
  } else if (auto address = boost::get<PciAddress>(&cardId)) {
    initWithAddress(*address);
  } else if (auto sequenceNumber = boost::get<PciSequenceNumber>(&cardId)) {
    initWithSequenceNumber(*sequenceNumber);
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not _parse_ Card ID"));
  }
}

RocPciDevice::~RocPciDevice()
{
}

void RocPciDevice::initWithSerialId(const SerialId& serialId)
{
  try {
    for (const auto& typedPciDevice : Pda::PdaDevice::getPciDevices()) {
      PciDevice* pciDevice = typedPciDevice.pciDevice;
      DeviceType type;

      Utilities::resetSmartPtr(mPdaBar0, pciDevice, 0);
      Utilities::resetSmartPtr(mPdaBar2, pciDevice, 2);

      int serial;
      int endpoint;
      if (typedPciDevice.cardType == CardType::Crorc) {
        type = DeviceType(deviceTypes.at(0));
        serial = type.getSerial(mPdaBar0);
      } else {
        type = DeviceType(deviceTypes.at(1));
        serial = type.getSerial(mPdaBar2);
      }
      endpoint = type.getEndpoint(mPdaBar0);

      if (serial == serialId.getSerial() && endpoint == serialId.getEndpoint()) {
        mPciDevice = pciDevice;
        mDescriptor = CardDescriptor{ type.cardType, serialId, type.pciId, addressFromDevice(pciDevice), PciDevice_getNumaNode(pciDevice) };
        return;
      }
    }
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not find card"));
  } catch (boost::exception& e) {
    e << ErrorInfo::SerialId(serialId);
    addPossibleCauses(e, { "Invalid serial and/or endpoint" });
    throw;
  }
}

void RocPciDevice::initWithAddress(const PciAddress& address)
{
  try {
    for (const auto& typedPciDevice : Pda::PdaDevice::getPciDevices()) {
      PciDevice* pciDevice = typedPciDevice.pciDevice;
      DeviceType type;
      if (addressFromDevice(pciDevice) == address) {
        Utilities::resetSmartPtr(mPdaBar0, pciDevice, 0);
        Utilities::resetSmartPtr(mPdaBar2, pciDevice, 2);

        int serial;
        int endpoint;
        if (typedPciDevice.cardType == CardType::Crorc) {
          type = DeviceType(deviceTypes.at(0));
          serial = type.getSerial(mPdaBar0);
        } else {
          type = DeviceType(deviceTypes.at(1));
          serial = type.getSerial(mPdaBar2);
        }
        endpoint = type.getEndpoint(mPdaBar0);

        mPciDevice = pciDevice;
        mDescriptor = CardDescriptor{ type.cardType, SerialId{ serial, endpoint }, type.pciId, address, PciDevice_getNumaNode(pciDevice) };
        return;
      }
    }
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not find card"));
  } catch (boost::exception& e) {
    e << ErrorInfo::PciAddress(address);
    addPossibleCauses(e, { "Invalid PCI address" });
    throw;
  }
}

void RocPciDevice::initWithSequenceNumber(const PciSequenceNumber& sequenceNumber)
{
  int sequenceCounter = 0;
  try {
    for (const auto& typedPciDevice : Pda::PdaDevice::getPciDevices()) {
      PciDevice* pciDevice = typedPciDevice.pciDevice;
      DeviceType type;
      if (sequenceNumber == sequenceCounter) {
        Utilities::resetSmartPtr(mPdaBar0, pciDevice, 0);
        Utilities::resetSmartPtr(mPdaBar2, pciDevice, 2);

        int serial;
        int endpoint;
        if (typedPciDevice.cardType == CardType::Crorc) {
          type = DeviceType(deviceTypes.at(0));
          serial = type.getSerial(mPdaBar0);
        } else {
          type = DeviceType(deviceTypes.at(1));
          serial = type.getSerial(mPdaBar2);
        }
        endpoint = type.getEndpoint(mPdaBar0);

        mPciDevice = pciDevice;
        mDescriptor = CardDescriptor{ type.cardType, SerialId{ serial, endpoint }, type.pciId, addressFromDevice(pciDevice), PciDevice_getNumaNode(pciDevice) };
        return;
      }
      sequenceCounter++;
    }
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not find card"));
  } catch (boost::exception& e) {
    e << ErrorInfo::PciSequenceNumber(sequenceNumber);
    addPossibleCauses(e, { "Invalid sequence number" });
    throw;
  }
}

std::vector<CardDescriptor> RocPciDevice::findSystemDevices()
{
  std::vector<CardDescriptor> cards;
  for (const auto& typedPciDevice : Pda::PdaDevice::getPciDevices()) {
    PciDevice* pciDevice = typedPciDevice.pciDevice;
    DeviceType type;

    std::shared_ptr<Pda::PdaBar> pdaBar0;
    Utilities::resetSmartPtr(pdaBar0, pciDevice, 0);
    std::shared_ptr<Pda::PdaBar> pdaBar2;
    Utilities::resetSmartPtr(pdaBar2, pciDevice, 2);

    int serial;
    int endpoint;
    if (typedPciDevice.cardType == CardType::Crorc) {
      type = DeviceType(deviceTypes.at(0));
      serial = type.getSerial(pdaBar0);
    } else {
      type = DeviceType(deviceTypes.at(1));
      serial = type.getSerial(pdaBar2);
    }
    endpoint = type.getEndpoint(pdaBar0);

    try {
      cards.push_back(CardDescriptor{ type.cardType, SerialId{ serial, endpoint }, type.pciId, addressFromDevice(pciDevice), PciDevice_getNumaNode(pciDevice) });
    } catch (boost::exception& e) {
      std::cout << boost::diagnostic_information(e);
    }
  }
  return cards;
}

void RocPciDevice::printDeviceInfo(std::ostream& ostream)
{
  uint16_t domainId;
  uint8_t busId;
  uint8_t functionId;
  const PciBarTypes* pciBarTypesPtr;

  if (PciDevice_getDomainID(mPciDevice, &domainId) || PciDevice_getBusID(mPciDevice, &busId) || PciDevice_getFunctionID(mPciDevice, &functionId) || PciDevice_getBarTypes(mPciDevice, &pciBarTypesPtr)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Failed to retrieve device info"));
  }

  auto barType = *pciBarTypesPtr;
  auto barTypeString =
    barType == PCIBARTYPES_NOT_MAPPED ? "NOT_MAPPED" : barType == PCIBARTYPES_IO ? "IO" : barType == PCIBARTYPES_BAR32 ? "BAR32" : barType == PCIBARTYPES_BAR64 ? "BAR64" : "n/a";

  auto f = boost::format("%-14s %10s\n");
  ostream << f % "Domain ID" << domainId;
  ostream << f % "Bus ID" << busId;
  ostream << f % "Function ID" << functionId;
  ostream << f % "BAR type" << barTypeString;
}

int cruGetSerial(std::shared_ptr<Pda::PdaBar> pdaBar2)
{
  int serial = CruBar(pdaBar2).getSerial().get();
  return serial;
}

int crorcGetSerial(std::shared_ptr<Pda::PdaBar> pdaBar0)
{
  return Crorc::getSerial(*pdaBar0.get()).get();
}

int cruGetEndpoint(std::shared_ptr<Pda::PdaBar> pdaBar0)
{
  return CruBar(pdaBar0).getEndpointNumber();
}

int crorcGetEndpoint(std::shared_ptr<Pda::PdaBar> /*pdaBar0*/)
{
  return 0;
}

} // namespace roc
} // namespace AliceO2
