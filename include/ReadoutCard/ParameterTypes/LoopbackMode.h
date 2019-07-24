// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file LoopbackMode.h
/// \brief Definition of the LoopbackMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_LOOPBACKMODE_H_
#define ALICEO2_INCLUDE_READOUTCARD_LOOPBACKMODE_H_

#include <string>

namespace AliceO2
{
namespace roc
{

/// Namespace for the ReadoutCard loopback mode enum, and supporting functions
struct LoopbackMode {
  /// Loopback mode
  enum type {
    None = 0,     ///< No loopback
    Diu = 1,      ///< Loopback through DIU (CRORC)
    Siu = 2,      ///< Loopback through SIU (CRORC)
    Internal = 3, ///< Internal loopback (CRORC)
    Ddg = 4,      ///< DDG (CRU)
  };

  /// Converts a LoopbackMode to a string
  static std::string toString(const LoopbackMode::type& mode);

  /// Converts a string to a LoopbackMode
  static LoopbackMode::type fromString(const std::string& string);

  /// Returns true if the loopback mode is external (SIU and/or DIU)
  static bool isExternal(const LoopbackMode::type& mode);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_LOOPBACKMODE_H_
