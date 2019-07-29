// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CardFinder.cxx
/// \brief Implementation of the CardFinder collection of functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/CardFinder.h"
#include "RocPciDevice.h"

namespace AliceO2
{
namespace roc
{

std::vector<CardDescriptor> findCards()
{
  std::vector<CardDescriptor> cards = RocPciDevice::findSystemDevices();
  return cards;
}

} // namespace roc
} // namespace AliceO2
