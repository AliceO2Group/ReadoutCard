// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

namespace AliceO2 {
namespace roc {

namespace {

  int parseSequenceNumberString(const std::string& string) {
    std::regex expression ("^[ \t]*#[0-9]+[ \t]*$");
    if (std::regex_search(string, expression)) {
      return stoi(string.substr(string.find('#') + 1));
    } else {
      BOOST_THROW_EXCEPTION(ParseException()
          << ErrorInfo::Message("Parsing PCI sequence number failed"));
    }
  }

}

PciSequenceNumber::PciSequenceNumber(const std::string& string)
{
  mSequenceNumberString = string;
  mSequenceNumber = parseSequenceNumberString(string);
}

std::string PciSequenceNumber::toString() const
{
  return mSequenceNumberString;
}

boost::optional<PciSequenceNumber> PciSequenceNumber::fromString(std::string string)
{
  try {
    return PciSequenceNumber(string);
  }
  catch (const ParseException& e) {
    return {};
  }
}

std::ostream& operator<<(std::ostream& os, const PciSequenceNumber& pciSequenceNumber)
{
  os << pciSequenceNumber.toString();
  return os;
}

} // namespace roc
} // namespace AliceO2
