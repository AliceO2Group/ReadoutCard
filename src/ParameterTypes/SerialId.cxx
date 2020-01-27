// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file SerialId.cxx
/// \brief Implementation of the SerialId struct.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/ParameterTypes/SerialId.h"
#include "ExceptionInternal.h"
#include <regex>

namespace AliceO2
{
namespace roc
{

namespace
{

bool parseSerialIdString(const std::string string, int& serial, int& endpoint)
{
  std::regex expression("^[ \t]*[0-9]{5}:?[0-1]?[ \t]*$");
  if (std::regex_search(string, expression)) {
    serial = stoi(string.substr(0, string.find(':'))); // Will return the SerialId struct
    endpoint = stoi(string.substr(string.find(':') + 1));
    if (endpoint == serial) {
      endpoint = 0;
    }
    return true;
  } else {
    BOOST_THROW_EXCEPTION(ParseException()
                          << ErrorInfo::Message("Parsing PCI Serial ID failed"));
  }

  return false;
}

void checkRanges(int serial, int endpoint)
{
  if ((serial < SERIAL_RANGE_LOW || serial > SERIAL_RANGE_HIGH) && serial != SERIAL_DUMMY) {
    BOOST_THROW_EXCEPTION(ParameterException()
                          << ErrorInfo::Message("Serial out of range")
                          << ErrorInfo::SerialIdSerial(serial));
  }

  if (endpoint != 0 && endpoint != 1 && endpoint != ENDPOINT_DUMMY) {
    BOOST_THROW_EXCEPTION(ParameterException()
                          << ErrorInfo::Message("Endpoint out of range")
                          << ErrorInfo::SerialIdEndpoint(endpoint));
  }
}

} // namespace

SerialId::SerialId(const std::string& string)
  : mSerialIdString(string)
{
  if (!parseSerialIdString(string, mSerial, mEndpoint)) {
    BOOST_THROW_EXCEPTION(ParseException()
                          << ErrorInfo::Message("Parsing PCI address failed"));
  }
  checkRanges(mSerial, mEndpoint);
}

SerialId::SerialId(int serial, int endpoint)
  : mSerial(serial),
    mEndpoint(endpoint),
    mSerialIdString(std::to_string(serial) + ":" + std::to_string(endpoint))
{
  checkRanges(mSerial, mEndpoint);
}

SerialId::SerialId(int serial)
  : mSerial(serial),
    mEndpoint(ENDPOINT_DEFAULT),
    mSerialIdString(std::to_string(serial) + ":0")
{
  checkRanges(mSerial, mEndpoint);
}

std::string SerialId::toString() const
{
  return mSerialIdString;
}

boost::optional<SerialId> SerialId::fromString(std::string string)
{
  try {
    return SerialId(string);
  } catch (const ParseException& e) {
    return {};
  }
}

int SerialId::getSerial() const
{
  return mSerial;
}

int SerialId::getEndpoint() const
{
  return mEndpoint;
}

std::ostream& operator<<(std::ostream& os, const SerialId& serialId)
{
  os << serialId.toString();
  return os;
}

} // namespace roc
} // namespace AliceO2
