// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DataSource.h
/// \brief Definition of the DataSource enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_DATASOURCE_H_
#define ALICEO2_INCLUDE_READOUTCARD_DATASOURCE_H_

#include <string>

namespace AliceO2
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
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_DATASOURCE_H_
