
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
/// \file ResetLevel.cxx
/// \brief Implementation of the ResetLevel enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ParameterTypes/ResetLevel.h"
#include "Utilities/Enum.h"

namespace o2
{
namespace roc
{
namespace
{

static const auto converter = Utilities::makeEnumConverter<ResetLevel::type>("ResetLevel", {
                                                                                             { ResetLevel::Nothing, "NOTHING" },
                                                                                             { ResetLevel::Internal, "INTERNAL" },
                                                                                             { ResetLevel::InternalSiu, "INTERNAL_SIU" },
                                                                                           });

} // Anonymous namespace

bool ResetLevel::includesExternal(const ResetLevel::type& mode)
{
  return mode == ResetLevel::InternalSiu;
}

std::string ResetLevel::toString(const ResetLevel::type& level)
{
  return converter.toString(level);
}

ResetLevel::type ResetLevel::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace o2
