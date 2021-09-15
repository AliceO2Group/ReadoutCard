
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
/// \file SerialId.h
/// \brief Definition of the SerialId class.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_SERIALID_H_
#define O2_READOUTCARD_INCLUDE_SERIALID_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <iostream>
#include <string>
#include <boost/optional.hpp>

namespace o2
{
namespace roc
{

/// Data holder class for a Serial Identifier, a serial and endpoint pair (e.g. serial:10241 endpoint:1)
class SerialId
{
 public:
  /// Constructs a SerialId object using a string in the sssss[:e] format
  /// For example: "10241", "10241:1"
  /// \param string String of format "^[ \t]*([0-9]{3}|[0-9]{4}|[0-9]{5}):?[0-1]?[ \t]*$"
  SerialId(const std::string& string);

  SerialId(int serial, int endpoint);

  SerialId(int serial);

  /// Converts to a SerialId object from a string that matches "^[ \t]*([0-9]{3}|[0-9]{4}|[0-9]{5}):?[0-1]?[ \t]*$"
  /// For example: "10241:0"
  static boost::optional<SerialId> fromString(std::string string);

  std::string toString() const;

  int getSerial() const;
  int getEndpoint() const;

  bool operator==(const std::string& other) const
  {
    return mSerialIdString == other;
  }

  bool operator==(const SerialId& other) const
  {
    return (mSerial == other.mSerial) && (mEndpoint == other.mEndpoint);
  }

  friend std::ostream& operator<<(std::ostream& os, const SerialId& serialId);

 private:
  int mSerial;
  int mEndpoint;
  std::string mSerialIdString;
};

//TODO: To be updated with sensible values when things are stable.
static constexpr int SERIAL_RANGE_LOW(0);
static constexpr int SERIAL_RANGE_HIGH(100000);

static constexpr int SERIAL_DUMMY(-1);
static constexpr int ENDPOINT_DUMMY(-1);
//static constexpr SerialId SERIAL_ID_DUMMY(SERIAL_DUMMY, ENDPOINT_DUMMY);

//static constexpr int SERIAL_DEFAULT(0);
static constexpr int ENDPOINT_DEFAULT(0);
//static constexpr SerialId SERIAL_ID_DEFAULT(SERIAL_DEFAULT, ENDPOINT_DEFAULT);

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_SERIALID_H_
