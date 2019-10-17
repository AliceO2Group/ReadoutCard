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

#ifndef ALICEO2_INCLUDE_READOUTCARD_FIRMWARECHECKER_H_
#define ALICEO2_INCLUDE_READOUTCARD_FIRMWARECHECKER_H_

#include "unordered_map"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Parameters.h"

namespace AliceO2
{
namespace roc
{

class FirmwareChecker
{
 public:
  FirmwareChecker();
  ~FirmwareChecker();

  std::string resolveFirmwareTag(std::string firmware);
  void checkFirmwareCompatibility(Parameters params);
  void checkFirmwareCompatibility(Parameters::CardIdType cardId);

 private:
  void checkFirmwareCompatibilityWrapped(std::shared_ptr<BarInterface> bar2);
  std::string getFirmwareCompatibilityList();
  std::unordered_map<std::string, std::string> mCompatibleFirmwareList;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_FIRMWARECHECKER_H_
