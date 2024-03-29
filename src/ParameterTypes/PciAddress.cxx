
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
/// \brief Implementation of the PciAddress struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include <iomanip>
#include <sstream>
#include <regex>
#include <boost/format.hpp>
#include <boost/spirit/include/qi.hpp>
#include "ExceptionInternal.h"

namespace o2
{
namespace roc
{

namespace
{
bool parseLspciFormat(const std::string& string, int& bus, int& slot, int& function)
{
  // Reject a case of the pci address + endpoint ID like "3b:00.0:1"
  std::regex expr(".*:.*:.*");
  if (std::regex_search(string, expr)) {
    return false;
  }

  using namespace boost::spirit::qi;
  return phrase_parse(string.begin(), string.end(), hex >> ":" >> hex >> "." >> hex, space, bus, slot, function);
}

void checkRanges(int bus, int slot, int function)
{
  if (bus < 0 || bus > 0xff) {
    BOOST_THROW_EXCEPTION(ParameterException()
                          << ErrorInfo::Message("Bus number out of range")
                          << ErrorInfo::PciAddressBusNumber(bus));
  }

  if (slot < 0 || slot > 0x1f) {
    BOOST_THROW_EXCEPTION(ParameterException()
                          << ErrorInfo::Message("Slot number out of range")
                          << ErrorInfo::PciAddressSlotNumber(slot));
  }

  if (function < 0 || function > 7) {
    BOOST_THROW_EXCEPTION(ParameterException()
                          << ErrorInfo::Message("Function number out of range")
                          << ErrorInfo::PciAddressFunctionNumber(function));
  }
}
} // Anonymous namespace

PciAddress::PciAddress(int bus, int slot, int function)
  : bus(bus), slot(slot), function(function)
{
  checkRanges(bus, slot, function);
}

PciAddress::PciAddress(const std::string& string)
  : bus(-1), slot(-1), function(-1)
{
  if (!parseLspciFormat(string, bus, slot, function)) {
    BOOST_THROW_EXCEPTION(ParseException()
                          << ErrorInfo::Message("Parsing PCI address failed"));
  }
  checkRanges(bus, slot, function);
}

std::string PciAddress::toString() const
{
  return boost::str(boost::format("%02x:%02x.%1x") % bus % slot % function);
}

boost::optional<PciAddress> PciAddress::fromString(std::string string)
{
  try {
    return PciAddress(string);
  } catch (const ParseException& e) {
    return {};
  }
}

std::ostream& operator<<(std::ostream& os, const PciAddress& pciAddress)
{
  os << pciAddress.toString();
  return os;
}

} // namespace roc
} // namespace o2
