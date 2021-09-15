
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
/// \file Eeprom.cxx
/// \brief Implementation of the Eeprom class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <regex>
#include "Constants.h"
#include "I2c.h"
#include "Eeprom.h"

namespace o2
{
namespace roc
{

Eeprom::Eeprom(std::shared_ptr<Pda::PdaBar> pdaBar) : mPdaBar(pdaBar)
{
}

std::string Eeprom::readContent()
{
  uint32_t chipAddress = 0x50; // fixed address
  I2c i2c = I2c(Cru::Registers::BSP_I2C_EEPROM.address, chipAddress, mPdaBar);

  i2c.resetI2c();

  std::string content;
  for (int i = 0; i < 1000 / 8; i++) { // EEPROM size is 1KB
    uint32_t res = i2c.readI2c(i);
    content += (char)res;
    if ((char)res == '}') {
      break;
    }
  }

  return content;
}

boost::optional<int32_t> Eeprom::getSerial()
{
  // parse the serial from the content
  const std::string content = readContent();

  // Example content:
  // {"cn": "FEDD", "dt": "2019-06-17", "io": "24/24", "pn": "p40_fv22b10241", "serial_number_p40": "18-02409 - 0136"}
  // p40_fv22b -> production CRUs
  // p40_tv20pr -> testing CRUs

  std::string startDelimiter = "\"pn\": \"p40_(?:fv22b|tv20pr)";
  std::string endDelimiter = "\", \"serial_number_p40\":";

  std::regex expression(startDelimiter + "(.*)" + endDelimiter);
  std::smatch match;

  if (std::regex_search(content.begin(), content.end(), match, expression)) {
    return std::stol(match[1], NULL, 0);
  }

  return {};
}

} // namespace roc
} // namespace o2
