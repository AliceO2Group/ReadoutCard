
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
/// \file Enum.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_UTILITIES_ENUM_H_
#define O2_READOUTCARD_SRC_UTILITIES_ENUM_H_

#include <stdexcept>
#include <utility>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/throw_exception.hpp>

namespace o2
{
namespace roc
{
namespace Utilities
{

template <typename Enum>
struct EnumConverter {
  const std::string typeName;
  const std::vector<std::pair<Enum, std::string>> mapping;

  std::string toString(Enum e) const
  {
    for (const auto& pair : mapping) {
      if (e == pair.first) {
        return pair.second;
      }
    }
    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to convert " + typeName + " enum to string"));
  }

  Enum fromString(const std::string& string) const
  {
    for (const auto& pair : mapping) {
      // Case-insensitive string comparison
      if (boost::iequals(string, pair.second)) {
        return pair.first;
      }
    }

    std::string errString = "Failed to convert string \"" + string + "\" to " + typeName + " enum\nPossible values:";
    for (const auto& el : mapping) {
      errString += " \"" + el.second + "\"";
    }

    BOOST_THROW_EXCEPTION(std::runtime_error(errString));
  }
};

template <typename Enum>
EnumConverter<Enum> makeEnumConverter(std::string typeName, std::vector<std::pair<Enum, std::string>> mapping)
{
  return EnumConverter<Enum>{ typeName, mapping };
}

} // namespace Utilities
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_UTILITIES_ENUM_H_
