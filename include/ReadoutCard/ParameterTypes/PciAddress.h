
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
/// \file PciAddress.h
/// \brief Definition of the PciAddress class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_PCIADDRESS_H_
#define O2_READOUTCARD_INCLUDE_PCIADDRESS_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <iostream>
#include <string>
#include <boost/optional.hpp>

namespace o2
{
namespace roc
{

/// Data holder class for a PCI address, consisting of a bus, slot and function number
class PciAddress
{
 public:
  /// Constructs a PciAddress object using a string in "lspci" format: "[bus]:[slot].[function]".
  /// For example: "01:23.0"
  /// \param string String with lspci format
  PciAddress(const std::string& string);

  /// Constructs a PciAddress object
  /// \param bus Bus number, allowed numbers: 0 to 255 (0xff)
  /// \param slot Slot number, allowed numbers: 0 to 31 (0x1f)
  /// \param function Function number, allowed numbers: 0 to 7
  PciAddress(int bus, int slot, int function);

  /// Converts a PciAddress object to a string in "lspci" format: "[bus]:[slot].[function]".
  /// For example: "01:23.0"
  std::string toString() const;

  /// Converts a PciAddress object from a string in "lspci" format: "[bus]:[slot].[function]".
  /// For example: "01:23.0"
  static boost::optional<PciAddress> fromString(std::string string);

  bool operator==(const PciAddress& other) const
  {
    return (bus == other.bus) && (slot == other.slot) && (function == other.function);
  }

  friend std::ostream& operator<<(std::ostream& os, const PciAddress& pciAddress);

  /// Gets the bus number of this address
  /// \return Integer from 0 to 255 (0xff)
  int getBus() const
  {
    return bus;
  }

  /// Gets the function number of this address
  /// \return Integer from 0 to 31 (0x1f)
  int getFunction() const
  {
    return function;
  }

  /// Gets the slot number of this address
  /// \return Integer from 0 to 7
  int getSlot() const
  {
    return slot;
  }

 private:
  int bus;
  int slot;
  int function;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_READOUTCARD_H_
