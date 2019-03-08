// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DatapathMode.h
/// \brief Definition of the DatapathMode enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CRU_DATAPATHMODE_H_
#define ALICEO2_INCLUDE_READOUTCARD_CRU_DATAPATHMODE_H_

#include <string>
#include "ReadoutCard/Cru.h"

namespace AliceO2 {
namespace roc {

/// Namespace for the ROC datapath mode enum, and supporting functions
struct DatapathMode
{
    enum type
    {
      Continuous = Cru::GBT_CONTINUOUS,
      Packet = Cru::GBT_PACKET,
    };

    /// Converts a DatapathMode to a string
    static std::string toString(const DatapathMode::type& datapathMode);

    /// Converts a string to a DatapathMode
    static DatapathMode::type fromString(const std::string& string);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_DATAPATHMODE_H_
