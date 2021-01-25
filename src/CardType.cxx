// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CardType.cxx
/// \brief Implementation of the CardType enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/CardType.h"
#include "Utilities/Enum.h"

namespace AliceO2
{
namespace roc
{
namespace
{

static const auto converter = Utilities::makeEnumConverter<CardType::type>("CardType", {
                                                                                         { CardType::Unknown, "UNKNOWN" },
                                                                                         { CardType::Crorc, "CRORC" },
                                                                                         { CardType::Cru, "CRU" },
                                                                                       });

} // Anonymous namespace

std::string CardType::toString(const CardType::type& type)
{
  return converter.toString(type);
}

CardType::type CardType::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2
