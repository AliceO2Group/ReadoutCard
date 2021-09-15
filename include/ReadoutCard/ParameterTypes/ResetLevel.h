
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
/// \file ResetLevel.h
/// \brief Definition of the ResetLevel enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_RESETLEVEL_H_
#define O2_READOUTCARD_INCLUDE_RESETLEVEL_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>

namespace o2
{
namespace roc
{

/// Namespace for the RORC reset level enum, and supporting functions
struct ResetLevel {
  enum type {
    Nothing = 0,     ///< No reset
    Internal = 1,    ///< Reset internally only (+DIU for the CRORC)
    InternalSiu = 2, ///< Reset internally, the DIU, and the SIU (n/a for the CRU)
  };

  /// Converts a ResetLevel to a string
  static std::string toString(const ResetLevel::type& level);

  /// Converts a string to a ResetLevel
  static ResetLevel::type fromString(const std::string& string);

  /// Returns true if the reset level includes external resets (SIU)
  static bool includesExternal(const ResetLevel::type& level);
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_RESETLEVEL_H_
