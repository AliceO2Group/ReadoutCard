
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
/// \file DataSource.h
/// \brief Definition of the DataSource enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_DATASOURCE_H_
#define O2_READOUTCARD_INCLUDE_DATASOURCE_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>

namespace o2
{
namespace roc
{

/// Namespace for the ReadoutCard data source enum, and supporting functions
struct DataSource {
  /// Data Source
  enum type {
    Fee = 0,      ///< FEE (CRU & CRORC)
    Diu = 1,      ///< DIU (CRORC)
    Siu = 2,      ///< SIU (CRORC)
    Internal = 3, ///< DG  (CRU & CRORC)
    Ddg = 4,      ///< DDG (CRU)
  };

  /// Converts a DataSource to a string
  static std::string toString(const DataSource::type& mode);

  /// Converts a string to a DataSource
  static DataSource::type fromString(const std::string& string);

  /// Returns true if the data source is external (FEE, DIU, SIU)
  static bool isExternal(const DataSource::type& mode);
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_DATASOURCE_H_
