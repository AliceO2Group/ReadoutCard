// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GbtMux.h
/// \brief Definition of the GbtMux enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMUX_H_
#define ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMUX_H_

#include <string>
#include "ReadoutCard/Cru.h"

namespace AliceO2
{
namespace roc
{

/// Namespace for the ROC GBT mux enum, and supporting functions
struct GbtMux {
  enum type {
    Ttc = Cru::GBT_MUX_TTC,
    Ddg = Cru::GBT_MUX_DDG,
    Swt = Cru::GBT_MUX_SWT,
    Na = -1,
  };

  /// Converts a GbtMux to an int
  static std::string toString(const GbtMux::type& gbtMux);

  /// Converts a string to a GbtMux
  static GbtMux::type fromString(const std::string& string);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMUX_H_
