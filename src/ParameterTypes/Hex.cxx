
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
/// \file Hex.cxx
/// \brief Implementation of the Hex struct and supporting functions
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/Hex.h"
#include "Utilities/Enum.h"

namespace o2
{
namespace roc
{

Hex::type Hex::fromString(const std::string& string)
{
  return std::stoul(string, nullptr, 16);
}

} // namespace roc
} // namespace o2
