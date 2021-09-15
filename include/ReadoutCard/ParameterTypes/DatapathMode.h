
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
/// \file DatapathMode.h
/// \brief Definition of the DatapathMode enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_CRU_DATAPATHMODE_H_
#define O2_READOUTCARD_INCLUDE_CRU_DATAPATHMODE_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>
#include "ReadoutCard/Cru.h"

namespace o2
{
namespace roc
{

/// Namespace for the ROC datapath mode enum, and supporting functions
struct DatapathMode {
  enum type {
    Streaming = Cru::GBT_STREAMING,
    Packet = Cru::GBT_PACKET,
  };

  /// Converts a DatapathMode to a string
  static std::string toString(const DatapathMode::type& datapathMode);

  /// Converts a string to a DatapathMode
  static DatapathMode::type fromString(const std::string& string);
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_CRU_DATAPATHMODE_H_
