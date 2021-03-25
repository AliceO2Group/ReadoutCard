// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DataSource.cxx
/// \brief Implementation of the DataSource enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/DataSource.h"
#include "Utilities/Enum.h"

namespace o2
{
namespace roc
{
namespace
{

static const auto converter = Utilities::makeEnumConverter<DataSource::type>("DataSource", {
                                                                                             { DataSource::Fee, "FEE" },
                                                                                             { DataSource::Internal, "INTERNAL" },
                                                                                             { DataSource::Diu, "DIU" },
                                                                                             { DataSource::Siu, "SIU" },
                                                                                             { DataSource::Ddg, "DDG" },
                                                                                           });

} // Anonymous namespace

bool DataSource::isExternal(const DataSource::type& mode)
{
  return mode == DataSource::Siu || mode == DataSource::Diu || mode == DataSource::Fee;
}

std::string DataSource::toString(const DataSource::type& mode)
{
  return converter.toString(mode);
}

DataSource::type DataSource::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace o2
