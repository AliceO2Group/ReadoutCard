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

#include "unordered_map"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Parameters.h"

namespace AliceO2
{
namespace roc
{

inline std::unordered_map<std::string, std::string> compatibleFirmwareList({ { "20190911-150139-3f5e11b3", "CRU v3.3.0" },
                                                                             { "20190718-120712-4c8e6c48", "CRU v3.2.0" },
                                                                             { "0.0:2000-0-0", "CRORC alpha" } });

inline std::string getFirmwareCompatibilityList()
{
  std::string fwStrings;
  for (auto fwString : compatibleFirmwareList) {
    fwStrings += "\n" + fwString.second + " - " + fwString.first;
  }
  return fwStrings;
}

inline void checkFirmwareCompatibilityWrapped(std::shared_ptr<BarInterface> bar2)
{
  auto firmware = bar2->getFirmwareInfo().value_or("");
  auto serial = bar2->getSerial().value_or(-1);
  if (compatibleFirmwareList.find(firmware) == compatibleFirmwareList.end()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(
                            std::string("Firmware compatibility check failed.\n") +
                            std::string("Serial: " + std::to_string(serial) + "\n") +
                            std::string("Firmware: " + firmware + "\n") +
                            std::string("\nCompatible firmwares:") +
                            getFirmwareCompatibilityList()));
  }
}

inline void checkFirmwareCompatibility(Parameters params)
{
  auto bar2 = ChannelFactory().getBar(params);
  checkFirmwareCompatibilityWrapped(bar2);
}

inline void checkFirmwareCompatibility(Parameters::CardIdType cardId)
{
  auto params = Parameters::makeParameters(cardId, 2); // access bar2 to check the firmware release
  auto bar2 = ChannelFactory().getBar(params);
  checkFirmwareCompatibilityWrapped(bar2);
}

} // namespace roc
} // namespace AliceO2
