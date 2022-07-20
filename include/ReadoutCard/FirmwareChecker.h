
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

#ifndef O2_READOUTCARD_INCLUDE_FIRMWARECHECKER_H_
#define O2_READOUTCARD_INCLUDE_FIRMWARECHECKER_H_

#include "ReadoutCard/NamespaceAlias.h"
#include "unordered_map"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Parameters.h"

namespace o2
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
  void checkFirmwareCompatibilityWrapped(Parameters::CardIdType cardId);
  std::string getFirmwareCompatibilityList();
  std::unordered_map<std::string, std::string> mCompatibleFirmwareList;
  static constexpr char kFirmwareListFile[] = "json:///etc/o2.d/readoutcard/o2-roc-fw-list.json";
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_FIRMWARECHECKER_H_
