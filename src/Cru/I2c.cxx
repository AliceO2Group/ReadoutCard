
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
/// \file I2c.cxx
/// \brief Implementation of the I2c class.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include <chrono>
#include <fstream>
#include <thread>
#include <cmath>
#include "I2c.h"
#include "Utilities/Util.h"

namespace o2
{
namespace roc
{

I2c::I2c(uint32_t baseAddress, uint32_t chipAddress,
         std::shared_ptr<BarInterface> bar,
         int endpoint,
         std::vector<std::pair<uint32_t, uint32_t>> registerMap)
  : mI2cConfig(baseAddress),
    mI2cCommand(baseAddress + 0x4),
    mI2cData(baseAddress + 0x10),
    mChipAddress(chipAddress),
    mChipAddressStart(0x0),
    mChipAddressEnd(0x7f),
    mBar(bar),
    mEndpoint(endpoint),
    mRegisterMap(registerMap)
{
}

I2c::~I2c()
{
}

void I2c::resetI2c()
{
  mBar->writeRegister(mI2cCommand / 4, 0x8);
  mBar->writeRegister(mI2cCommand / 4, 0x0);
}

void I2c::configurePll()
{
  //for (auto const& reg: mRegisterMap)
  //  std::cout << "registerMap [ " << reg.first << "] [" << reg.second << "]" << std::endl;

  resetI2c();
  // switch to page 0
  writeI2c(0x01, 0);

  // TODO(from cru-sw): check device ready at 0x00fe
  uint32_t currentPage = 0;
  uint32_t nextPage = 0;

  for (auto const& reg : mRegisterMap) {
    // change page if needed
    nextPage = reg.first >> 8;
    if (nextPage != currentPage) {
      resetI2c();
      writeI2c(0x01, nextPage);
      currentPage = nextPage;
    }
    resetI2c();
    writeI2c(reg.first & 0xff, reg.second);

    if (reg.first == 0x0540) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  resetI2c();
}

uint32_t I2c::getSelectedClock()
{
  resetI2c();
  writeI2c(0x0001, 0x5); //TODO: magic number
  uint32_t clock = readI2c(0x2A) >> 1 & 0x3;
  return clock;
}

/*std::map<uint32_t, uint32_t> I2c::readRegisterMap(std::string file)
{
  //std::cout << "readRegisterMap: Trying to open" << file << std::endl;
  std::string line;
  std::string delimiter = ",";
  std::ifstream regFile(file);
  std::map<uint32_t, uint32_t> registerMap;
  if (regFile.is_open()) {
    while(getline(regFile, line)) {
      if ((line.find("#") != std::string::npos) || (line.find("Address") != std::string::npos)) {
        continue;
      }

      std::string addressString = line.substr(0, line.find(","));
      std::string dataString = line.substr(line.find(",") + 1);
      
      //std::cout << "Read: " << addressString << std::endl;
      //std::cout << dataString << std::endl;
    std::this_thread::sleep_for(std::chrono::nanoseconds(200));

      uint32_t address = strtoul(addressString.c_str(), NULL, 16);
      uint32_t data = strtoul(dataString.c_str(), NULL, 16);

      //std::cout << "Read [0x" << std::hex << address << "] [0x" << std::hex << data << "]" << std::endl;

      registerMap.insert(std::make_pair(address, data));
    }
    regFile.close();
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("readRegisterMap: Cannot open file for reading"));
  }

  return registerMap;
}*/

void I2c::writeI2c(uint32_t address, uint32_t data)
{
  uint32_t value = (mChipAddress << 16) | (address << 8) | data;
  mBar->writeRegister(mI2cConfig / 4, value);

  mBar->writeRegister(mI2cCommand / 4, 0x1);
  mBar->writeRegister(mI2cCommand / 4, 0x0);

  waitForI2cReady();
}

uint32_t I2c::readI2c(uint32_t address)
{
  uint32_t readCommand = (mChipAddress << 16) + (address << 8) + 0x0;
  mBar->writeRegister(mI2cConfig / 4, readCommand);

  mBar->writeRegister(mI2cCommand / 4, 0x2);
  mBar->writeRegister(mI2cCommand / 4, 0x0);

  waitForI2cReady();
  uint32_t readValue = mBar->readRegister(mI2cData / 4);
  readValue &= 0xff;

  return readValue;
}

void I2c::waitForI2cReady()
{
  int done = 0;
  uint32_t readValue = 0;
  while (readValue == 0 && done < 10) {
    readValue = mBar->readRegister(mI2cData / 4);
    readValue = Utilities::getBit(readValue, 31);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    done++;
  }
}

std::vector<uint32_t> I2c::getChipAddresses()
{
  std::vector<uint32_t> chipAddresses;

  for (uint32_t addr = mChipAddressStart; addr <= mChipAddressEnd; addr++) { //Get valid chip addresses
    resetI2c();
    uint32_t val32 = ((addr << 16) | 0x0);
    mBar->writeRegister(mI2cConfig / 4, int(val32)); //so far so good

    mBar->writeRegister(mI2cCommand / 4, 0x4);
    mBar->writeRegister(mI2cCommand / 4, 0x0);

    waitForI2cReady();

    uint32_t addrValue = mBar->readRegister(mI2cData / 4);
    //addrValue = addrValue < 0 ? addrValue + pow(2, 32) : addrValue; //unsigned is never negative : what was the idea here?
    if ((addrValue >> 31) == 0x1) {
      chipAddresses.push_back(addr);
    }
  }

  return chipAddresses;
}

void I2c::getOpticalPower(std::map<int, Link>& linkMap)
{
  std::vector<uint32_t> chipAddresses = getChipAddresses();
  std::vector<float> opticalPowers;

  for (uint32_t chipAddr : chipAddresses) {
    // Open I2c for specific chip addr
    I2c minipod = I2c(Cru::Registers::BSP_I2C_MINIPODS.address, chipAddr, mBar);

    minipod.resetI2c();

    // Check that it is RX, if not continue
    uint32_t type = minipod.readI2c(177); //?...
    if (type != 50) {                     //minipod is not RX, we don't care about it
      continue;
    }

    // Read the reg values and apply the necessary function for the optical power
    for (uint32_t regAddress = 64; regAddress < 88; regAddress += 2) {
      uint32_t reg0 = minipod.readI2c(regAddress);
      uint32_t reg1 = minipod.readI2c(regAddress + 1);

      float opticalPower = (((reg0 << 8) + reg1) * 0.1); //is f() in cru-sw
      opticalPowers.push_back(opticalPower);
    }
  }

  /* opticalPowers contains 48 elements, 4chips * 12links. We only care for the first two chips
   * chip0 -> links 0-11
   * chip1 -> links 12-23
   * (chip2 & chip3 don't concern us)
   * *but* links are reversed so value 0  -> chip0 but link 11
   *                             value 1  -> chip0 but link 10
   *                             ...
   *                             value 11 -> chip0 but link 0
   *                             value 12 -> chip1 but link 11
   *                             ...
   *                             value 23 -> chip1 but link 23
   *                             ... */

  int chipIndex = (mEndpoint == 0) ? 11 : 23;
  for (auto& el : linkMap) {
    auto& link = el.second;
    if (opticalPowers.empty()) { //Means no chip found
      link.opticalPower = 0.0;
    } else {
      link.opticalPower = opticalPowers[chipIndex--];
    }
  }

  return;
}

double I2c::getRxPower()
{
  resetI2c();
  uint32_t address = 0x68;
  uint32_t rxPower = readI2c(address) << 8;
  rxPower |= readI2c(address + 1); // unit: 0.1*uW
  if (rxPower) {
    double watts = rxPower * 0.1 * 1e-6;
    return 10 * log10(watts / 1e-3); // convert to dBm
  } else {
    return -1 * std::numeric_limits<double>::infinity();
  }
}

} // namespace roc
} // namespace o2
