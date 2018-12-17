/// \file I2c.cxx
/// \brief Implementation of the I2c class.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include <chrono>
#include <fstream>
#include <thread>
#include "I2c.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {

I2c::I2c(uint32_t baseAddress, uint32_t chipAddress, 
    std::shared_ptr<Pda::PdaBar> pdaBar,
    std::vector<std::pair<uint32_t, uint32_t>> registerMap)
  : mI2cConfig(baseAddress),
    mI2cCommand(baseAddress + 0x4),
    mI2cData(baseAddress + 0x10),
    mChipAddress(chipAddress),
    mChipAddressStart(0x0),
    mChipAddressEnd(0x7f),
    mPdaBar(pdaBar),
    mRegisterMap(registerMap)
{
}

I2c::~I2c()
{
}

void I2c::resetI2c()
{
  mPdaBar->writeRegister(mI2cCommand/4, 0x8);
  mPdaBar->writeRegister(mI2cCommand/4, 0x0);
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

  for (auto const& reg: mRegisterMap) {
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
  mPdaBar->writeRegister(mI2cConfig/4, value);

  mPdaBar->writeRegister(mI2cCommand/4, 0x1);
  std::this_thread::sleep_for(std::chrono::nanoseconds(100));
  mPdaBar->writeRegister(mI2cCommand/4, 0x0);

  waitForI2cReady();
}

uint32_t I2c::readI2c(uint32_t address)
{
  uint32_t readCommand = (mChipAddress << 16) + (address << 8) + 0x0;
  mPdaBar->writeRegister(mI2cConfig/4, readCommand);
  
  mPdaBar->writeRegister(mI2cCommand/4, 0x2);
  mPdaBar->writeRegister(mI2cCommand/4, 0x0);

  waitForI2cReady();
  uint32_t readValue = mPdaBar->readRegister(mI2cData/4);
  readValue &= 0xff;

  return readValue;
}

void I2c::waitForI2cReady()
{
  int done = 0;
  uint32_t readValue = 0;
  while(readValue == 0 && done < 10) {
    readValue = mPdaBar->readRegister(mI2cData/4);
    readValue = Utilities::getBit(readValue, 31);
    std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    done++;
  }
}

} // namespace roc
} // namespace AliceO2
