// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ReadoutMode.h
/// \brief Definition of the ReadoutMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_READOUTMODE_H_
#define ALICEO2_INCLUDE_READOUTCARD_READOUTMODE_H_

#include <string>

namespace AliceO2 {
namespace roc {

/// Namespace for the RORC readout mode enum, and supporting functions
struct ReadoutMode
{
    /// Readout mode
    enum type
    {
      Continuous,
      //Triggered // Not yet supported
    };

    /// Converts a ReadoutMode to a string
    static std::string toString(const ReadoutMode::type& mode);

    /// Converts a string to a ReadoutMode
    static ReadoutMode::type fromString(const std::string& string);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_READOUTMODE_H_
