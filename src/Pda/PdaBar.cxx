
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
/// \file PdaBar.cxx
/// \brief Implementation of the PdaBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "PdaBar.h"

#include <limits>
#include <string>
#include <boost/lexical_cast.hpp>

#include "ReadoutCard/Exception.h"

namespace o2
{
namespace roc
{
namespace Pda
{

PdaBar::PdaBar() : mPdaBar(nullptr), mBarLength(-1), mBarNumber(-1), mUserspaceAddress(0)
{
}

PdaBar::PdaBar(PciDevice* pciDevice, int barNumberInt) : mBarNumber(barNumberInt)
{
  uint8_t barNumber = mBarNumber;

  if (mBarNumber > std::numeric_limits<decltype(barNumber)>::max()) {
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("BAR number out of range (max 256)")
                          << ErrorInfo::ChannelNumber(barNumber));
  }

  // Getting the BAR struct
  if (PciDevice_getBar(pciDevice, &mPdaBar, barNumber) != PDA_SUCCESS) {
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Failed to get BAR")
                          << ErrorInfo::ChannelNumber(barNumber));
  }

  // Mapping the BAR starting  address
  void* address = nullptr;
  if (Bar_getMap(mPdaBar, &address, &mBarLength) != PDA_SUCCESS) {
    BOOST_THROW_EXCEPTION(Exception()
                          << ErrorInfo::Message("Failed to map BAR")
                          << ErrorInfo::ChannelNumber(barNumber));
  }
  mUserspaceAddress = reinterpret_cast<uintptr_t>(address);
}

} // namespace Pda
} // namespace roc
} // namespace o2
