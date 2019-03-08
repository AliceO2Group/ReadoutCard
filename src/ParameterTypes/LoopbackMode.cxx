// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file LoopbackMode.cxx
/// \brief Implementation of the LoopbackMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ParameterTypes/LoopbackMode.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<LoopbackMode::type>("LoopbackMode", {
  { LoopbackMode::None, "NONE" },
  { LoopbackMode::Internal, "INTERNAL" },
  { LoopbackMode::Diu, "DIU" },
  { LoopbackMode::Siu, "SIU" },
  { LoopbackMode::Ddg, "DDG" },
});

} // Anonymous namespace

bool LoopbackMode::isExternal(const LoopbackMode::type& mode)
{
  return mode == LoopbackMode::Siu || mode == LoopbackMode::Diu || mode == LoopbackMode::None;
}

std::string LoopbackMode::toString(const LoopbackMode::type& mode)
{
  return converter.toString(mode);
}

LoopbackMode::type LoopbackMode::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2

