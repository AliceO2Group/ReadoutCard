// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ResetLevel.cxx
/// \brief Implementation of the ResetLevel enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ParameterTypes/GeneratorPattern.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<GeneratorPattern::type>("GeneratorPattern", {
  { GeneratorPattern::Alternating,  "ALTERNATING" },
  { GeneratorPattern::Constant,     "CONSTANT" },
  { GeneratorPattern::Decremental,  "DECREMENTAL" },
  { GeneratorPattern::Flying0,      "FLYING_0" },
  { GeneratorPattern::Flying1,      "FLYING_1" },
  { GeneratorPattern::Incremental,  "INCREMENTAL" },
  { GeneratorPattern::Random,       "RANDOM" },
  { GeneratorPattern::Unknown,      "UNKNOWN" },
});

} // Anonymous namespace

std::string GeneratorPattern::toString(const GeneratorPattern::type& level)
{
  return converter.toString(level);
}

GeneratorPattern::type GeneratorPattern::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2

