// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

namespace AliceO2
{
namespace roc
{

FirmwareChecker::FirmwareChecker() : mCompatibleFirmwareList({ /* CRU */
                                                               { "d458317e", "v3.5.2" },
                                                               { "6baf11da", "v3.5.1" },
                                                               { "5b162edd", "v3.5.0" },
                                                               { "51882687", "v3.4.0" },
                                                               { "3f5e11b3", "v3.3.0" },
                                                               { "4c8e6c48", "v3.2.0" },
                                                               /* CRORC */
                                                               { "0", "alpha" } })
{
}

FirmwareChecker::~FirmwareChecker()
{
}

std::string FirmwareChecker::resolveFirmwareTag(std::string firmware)
{
  firmware = firmware.substr(firmware.find_last_of("-") + 1);
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

void FirmwareChecker::checkFirmwareCompatibilityWrapped(std::shared_ptr<BarInterface> bar2)
{
  auto firmware = bar2->getFirmwareInfo().value_or("");
  firmware = firmware.substr(firmware.find_last_of("-") + 1);
  auto serial = bar2->getSerial().value_or(-1);
  if (mCompatibleFirmwareList.find(firmware) == mCompatibleFirmwareList.end()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(
                            std::string("Firmware compatibility check failed.\n") +
                            std::string("Serial: " + std::to_string(serial) + "\n") +
                            std::string("Firmware: " + firmware + "\n") +
                            std::string("\nCompatible firmwares:") +
                            getFirmwareCompatibilityList()));
  }
}

void FirmwareChecker::checkFirmwareCompatibility(Parameters params)
{
  auto bar2 = ChannelFactory().getBar(params);
  checkFirmwareCompatibilityWrapped(bar2);
}

void FirmwareChecker::checkFirmwareCompatibility(Parameters::CardIdType cardId)
{
  auto params = Parameters::makeParameters(cardId, 2); // access bar2 to check the firmware release
  auto bar2 = ChannelFactory().getBar(params);
  checkFirmwareCompatibilityWrapped(bar2);
}

} // namespace roc
} // namespace AliceO2
