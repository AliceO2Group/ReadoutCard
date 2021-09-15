
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
/// \file DownstreamData.h
/// \brief Definition of the DownstreamData enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_CRU_DOWNSTREAMDATA_H_
#define O2_READOUTCARD_INCLUDE_CRU_DOWNSTREAMDATA_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>
#include "ReadoutCard/Cru.h"

namespace o2
{
namespace roc
{

/// Namespace for the ROC GBT mux enum, and supporting functions
struct DownstreamData {
  enum type {
    Ctp = Cru::DATA_CTP,
    Pattern = Cru::DATA_PATTERN,
    Midtrg = Cru::DATA_MIDTRG,
  };

  /// Converts a DownstreamData to an int
  static std::string toString(const DownstreamData::type& downstreamData);

  /// Converts a string to a DownstreamData
  static DownstreamData::type fromString(const std::string& string);
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_CRU_DOWNSTREAMDATA_H_
