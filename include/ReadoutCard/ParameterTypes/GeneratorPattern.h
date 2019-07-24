// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GeneratorPattern.h
/// \brief Definition of the GeneratorPattern enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_GENERATORPATTERN_H_
#define ALICEO2_INCLUDE_READOUTCARD_GENERATORPATTERN_H_

#include <string>

namespace AliceO2
{
namespace roc
{

/// Namespace for the RORC generator pattern enum
struct GeneratorPattern {
  enum type {
    Unknown = 0,
    Constant = 1,
    Alternating = 2,
    Flying0 = 3,
    Flying1 = 4,
    Incremental = 5,
    Decremental = 6,
    Random = 7
  };

  /// Converts a GeneratorPattern to a string
  static auto toString(const GeneratorPattern::type& type) -> std::string;

  /// Converts a string to a GeneratorPattern
  static auto fromString(const std::string& string) -> GeneratorPattern::type;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_GENERATORPATTERN_H_
