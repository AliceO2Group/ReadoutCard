// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GbtPatternMode.h
/// \brief Definition of the GbtPatternMode enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_CRU_GBTPATTERNMODE_H_
#define O2_READOUTCARD_INCLUDE_CRU_GBTPATTERNMODE_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>

namespace o2
{
namespace roc
{

/// Namespace for the ROC GBT pattern mode enum, and supporting functions
struct GbtPatternMode {
  enum type {
    Counter,
    Static,
  };

  /// Converts a GbtPatternMode to an int
  static std::string toString(const GbtPatternMode::type& gbtPatternMode);

  /// Converts a string to a GbtPatternMode
  static GbtPatternMode::type fromString(const std::string& string);
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_CRU_GBTPATTERNMODE_H_
