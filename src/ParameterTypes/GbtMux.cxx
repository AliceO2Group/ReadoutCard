// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GbtMux.cxx
/// \brief Implementation of the GbtMux enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/GbtMux.h"
#include "Utilities/Enum.h"

namespace AliceO2
{
namespace roc
{
namespace
{

static const auto converter = Utilities::makeEnumConverter<GbtMux::type>("GbtMux", {
                                                                                     { GbtMux::Ttc, "TTC" },
                                                                                     { GbtMux::Ddg, "DDG" },
                                                                                     { GbtMux::Swt, "SWT" },
                                                                                     { GbtMux::Na, "N/A" },
                                                                                   });

} // Anonymous namespace

std::string GbtMux::toString(const GbtMux::type& gbtMux)
{
  return converter.toString(gbtMux);
}

GbtMux::type GbtMux::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2
