
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
/// \file PciSequenceNumber.h
/// \brief Definition of the PciSequenceNumber class.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_PCISEQUENCENUMBER_H_
#define O2_READOUTCARD_INCLUDE_PCISEQUENCENUMBER_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <iostream>
#include <string>
#include <boost/optional.hpp>

namespace o2
{
namespace roc
{

/// Data holder class for a PCI Sequence Number, a string starting with # (e.g. #01, #4)
class PciSequenceNumber
{
 public:
  /// Constructs a PciSequenceNumber object using a string in the #xxx format
  /// For example: "#01", "#4", "#0"
  /// \param string String of format "^#[0-9]+$"
  PciSequenceNumber(const std::string& string);

  /// Constructs a PciSequenceNumber object using an int
  /// For example: 1, 4, 0
  /// \param number An int between 0-7
  PciSequenceNumber(const int& number);

  /// Converts to a PciSequenceNumber object from a string that matches "^#[0-9]+$"
  /// For example: "#04"
  static boost::optional<PciSequenceNumber> fromString(std::string string);

  std::string toString() const;

  bool operator==(const int& other) const
  {
    return mSequenceNumber == other;
  }

  bool operator==(const PciSequenceNumber& other) const
  {
    return mSequenceNumber == other.mSequenceNumber;
  }

  friend std::ostream& operator<<(std::ostream& os, const PciSequenceNumber& pciSequenceNumber);

 private:
  int mSequenceNumber;
  std::string mSequenceNumberString;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_PCISEQUENCENUMBER_H_
