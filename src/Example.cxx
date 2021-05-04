// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Example.cxx
/// \brief Example usage of the BAR interface
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include "ReadoutCard/ChannelFactory.h"

using namespace o2;

int main()
{
  // See https://github.com/AliceO2Group/ReadoutCard#addressing for other addressing options
  auto parameters = roc::Parameters()
                      .setCardId(roc::PciSequenceNumber{ "#1" })
                      .setChannelNumber(2); // BAR numbre, Should always be 2
  auto bar = o2::roc::ChannelFactory().getBar(parameters);

  uint32_t address = 0x00260004;

  bar->writeRegister(address / 4, 0x42);
  auto reg = bar->readRegister(address / 4);
  if (reg == 0x42) {
    std::cout << "SUCCESS" << std::endl;
    return 0;
  }

  std::cout << "FAILURE" << std::endl;
  return 1;
}
