
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
/// \file I2c.h
/// \brief Definition of the I2c class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_I2C_H_
#define O2_READOUTCARD_CRU_I2C_H_

#include <map>
#include "Common.h"
#include "ReadoutCard/BarInterface.h"

namespace o2
{
namespace roc
{

class I2c
{

  using Link = Cru::Link;

 public:
  I2c(uint32_t baseAddress, uint32_t chipAddress,
      std::shared_ptr<BarInterface> pdaBar,
      int endpoint = 0,
      std::vector<std::pair<uint32_t, uint32_t>> registerMap = {});
  ~I2c();

  void resetI2c();
  void configurePll();
  uint32_t getSelectedClock();
  void getOpticalPower(std::map<int, Link>& linkMap);
  double getRxPower(); //unit: dBm
  uint32_t readI2c(uint32_t address);

 private:
  //std::map<uint32_t, uint32_t> readRegisterMap(std::string file);
  void writeI2c(uint32_t address, uint32_t data);
  void waitForI2cReady();
  std::vector<uint32_t> getChipAddresses();

  uint32_t mI2cConfig;
  uint32_t mI2cCommand;
  uint32_t mI2cData;

  uint32_t mChipAddress;
  uint32_t mChipAddressStart = 0x00;
  uint32_t mChipAddressEnd = 0x7f;

  std::shared_ptr<BarInterface> mBar;
  int mEndpoint = 0;
  std::vector<std::pair<uint32_t, uint32_t>> mRegisterMap;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_I2C_H_
