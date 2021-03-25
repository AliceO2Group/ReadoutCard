// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DownstreamData.cxx
/// \brief Implementation of the DownstreamData enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/DownstreamData.h"
#include "Utilities/Enum.h"

namespace o2
{
namespace roc
{
namespace
{

static const auto converter = Utilities::makeEnumConverter<DownstreamData::type>("DownstreamData", {
                                                                                                     { DownstreamData::Ctp, "Ctp" },
                                                                                                     { DownstreamData::Pattern, "Pattern" },
                                                                                                     { DownstreamData::Midtrg, "Midtrg" },
                                                                                                   });

} // Anonymous namespace

std::string DownstreamData::toString(const DownstreamData::type& downstreamData)
{
  return converter.toString(downstreamData);
}

DownstreamData::type DownstreamData::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace o2
