// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Address.cxx
/// \brief Implementation of the Address struct and supporting functions
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/Address.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {

Address::type Address::fromString(const std::string& string)
{
  return std::stoul(string, nullptr, 16);
}

} // namespace roc
} // namespace AliceO2
