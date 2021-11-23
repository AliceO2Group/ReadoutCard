
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file FirmwareChecker.h
/// \brief Helper functions to check a card's firmware compatibilty
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/FirmwareChecker.h"
#include "ExceptionInternal.h"

namespace o2
{
namespace roc
{

FirmwareChecker::FirmwareChecker() : mCompatibleFirmwareList({
                                       /* CRU */
                                       { "4a412c71", "v3.13.0" },
                                       /*{ "6a85d30c", "v3.12.0" },
                                       { "7be5aa1c", "v3.11.0" },
                                       { "e4a5a46e", "v3.10.0" },
                                       { "f71faa86", "v3.9.1" },
                                       { "8e0d2ffa", "v3.9.0" },
                                       { "0c6a3a7c", "MFT PSU" },
                                       { "e8e58cff", "v3.8.0" },
                                                               { "f8cecade", "v3.7.0" },
                                                               { "75b96268", "v3.6.1" },
                                                               { "6955404", "v3.6.0" },
                                                               { "d458317e", "v3.5.2" },
                                                               { "6baf11da", "v3.5.1" },*/
                                       /* CRORC */
                                       { "59e9955", "v2.8.1" },
                                       { "f086417", "v2.8.0" },
                                       { "474f9e1", "v2.7.0" }
                                       /*{ "8e3a98e", "v2.6.1" },
                                                               { "72cdb92", "v2.4.1" }*/
                                     })
{
}

FirmwareChecker::~FirmwareChecker()
{
}

std::string FirmwareChecker::resolveFirmwareTag(std::string firmware)
{
  //firmware = firmware.substr(firmware.find_last_of("-") + 1);
  if (mCompatibleFirmwareList.find(firmware) != mCompatibleFirmwareList.end()) {
    return mCompatibleFirmwareList.at(firmware);
  } else {
    return firmware;
  }
}

std::string FirmwareChecker::getFirmwareCompatibilityList()
{
  std::string fwStrings;
  for (auto fwString : mCompatibleFirmwareList) {
    fwStrings += "\n" + fwString.second + " - " + fwString.first;
  }
  return fwStrings;
}

void FirmwareChecker::checkFirmwareCompatibilityWrapped(Parameters::CardIdType cardId)
{
  auto bar0 = ChannelFactory().getBar(cardId, 0);
  auto bar2 = ChannelFactory().getBar(cardId, 2);
  auto firmware = bar2->getFirmwareInfo().value_or("");
  auto serial = bar2->getSerial().value_or(-1);
  auto endpoint = bar0->getEndpointNumber();
  if (mCompatibleFirmwareList.find(firmware) == mCompatibleFirmwareList.end()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(
                            std::string("Firmware compatibility check failed.\n") +
                            std::string("Serial: " + std::to_string(serial) + "\n") +
                            std::string("Endpoint: " + std::to_string(endpoint) + "\n") +
                            std::string("Firmware: " + firmware + "\n") +
                            std::string("\nCompatible firmwares:") +
                            getFirmwareCompatibilityList()));
  }
}

void FirmwareChecker::checkFirmwareCompatibility(Parameters params)
{
  checkFirmwareCompatibilityWrapped(params.getCardIdRequired());
}

void FirmwareChecker::checkFirmwareCompatibility(Parameters::CardIdType cardId)
{
  checkFirmwareCompatibilityWrapped(cardId);
}

} // namespace roc
} // namespace o2
