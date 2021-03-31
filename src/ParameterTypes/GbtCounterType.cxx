// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GbtCounterType.cxx
/// \brief Implementation of the GbtCounterType enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/GbtCounterType.h"
#include "Utilities/Enum.h"

namespace o2
{
namespace roc
{
namespace
{

static const auto converter = Utilities::makeEnumConverter<GbtCounterType::type>("GbtCounterType", {
                                                                                                     { GbtCounterType::ThirtyBit, "30bit" },
                                                                                                     { GbtCounterType::EightBit, "8bit" },
                                                                                                   });
} // Anonymous namespace

std::string GbtCounterType::toString(const GbtCounterType::type& gbtCounterType)
{
  return converter.toString(gbtCounterType);
}

GbtCounterType::type GbtCounterType::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace o2
