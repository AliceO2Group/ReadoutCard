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

/// \file Eeprom.h
/// \brief Definition of the Eeprom class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_EEPROM_H_
#define O2_READOUTCARD_CRU_EEPROM_H_

#include "Pda/PdaBar.h"

namespace o2
{
namespace roc
{

class Eeprom
{
 public:
  Eeprom(std::shared_ptr<Pda::PdaBar> pdaBar);

  boost::optional<int32_t> getSerial();

 private:
  std::string readContent();
  std::shared_ptr<Pda::PdaBar> mPdaBar;
};
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_EEPROM_H_
