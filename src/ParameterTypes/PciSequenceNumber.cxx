
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
/// \file PciSequenceNumber.cxx
/// \brief Implementation of the PciSequenceNumber struct.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/PciSequenceNumber.h"
#include "ExceptionInternal.h"
#include <regex>

namespace o2
{
namespace roc
{

namespace
{

int parseSequenceNumberString(const std::string& string)
{
  std::regex expression("^[ \t]*#[0-7]+[ \t]*$");
  if (std::regex_search(string, expression)) {
    return stoi(string.substr(string.find('#') + 1));
  } else {
    BOOST_THROW_EXCEPTION(ParseException()
                          << ErrorInfo::Message("Parsing PCI sequence number failed"));
  }
}

std::string constructSequenceNumberString(const int& number)
{
  if (number < 0 || number > 7) {
    BOOST_THROW_EXCEPTION(ParseException()
                          << ErrorInfo::Message("Parsing PCI sequence number failed"));
  }

  return std::string("#" + std::to_string(number));
}

} // namespace

PciSequenceNumber::PciSequenceNumber(const std::string& string)
{
  mSequenceNumberString = string;
  mSequenceNumber = parseSequenceNumberString(string);
}

PciSequenceNumber::PciSequenceNumber(const int& number)
{
  mSequenceNumberString = constructSequenceNumberString(number);
  mSequenceNumber = number;
}

std::string PciSequenceNumber::toString() const
{
  return mSequenceNumberString;
}

boost::optional<PciSequenceNumber> PciSequenceNumber::fromString(std::string string)
{
  try {
    return PciSequenceNumber(string);
  } catch (const ParseException& e) {
    return {};
  }
}

std::ostream& operator<<(std::ostream& os, const PciSequenceNumber& pciSequenceNumber)
{
  os << pciSequenceNumber.toString();
  return os;
}

} // namespace roc
} // namespace o2
