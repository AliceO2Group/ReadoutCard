
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
/// \file ExceptionInternal.cxx
/// \brief Implementation of internal ReadoutCard exceptions and related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ExceptionInternal.h"
#include <stdexcept>
#include <string>
#include <sstream>
#include <boost/exception/all.hpp>
#include <boost/format.hpp>
#include <cstdint>
#include "ReadoutCard/CardType.h"

using namespace o2::roc;
namespace b = boost;

namespace o2
{
namespace roc
{

void addPossibleCauses(boost::exception& exception, const std::vector<std::string>& newCauses)
{
  if (auto oldCauses = boost::get_error_info<ErrorInfo::PossibleCauses>(exception)) {
    // If we already had some possible causes, we add the new ones to them
    // Note that we also preserve the chronological order, so messages which are closest to the original throw
    // site -- which might even be more likely to reveal the underlying issue -- appear first in the diagnostic output.
    oldCauses->insert(oldCauses->end(), newCauses.begin(), newCauses.end());
  } else {
    // Otherwise, we can just them to the exception as usual
    exception << ErrorInfo::PossibleCauses(newCauses);
  }
}

} // namespace roc
} // namespace o2

template <typename Message>
std::string toStringHelper(const std::string& name, const Message& message)
{
  return b::str(b::format("[%s] = %s\n") % name % message);
}

template <typename Basic, typename Extended>
std::string toStringHelper(const std::string& name, const Basic& basic, const Extended& extended)
{
  return b::str(b::format("[%s] = %s, \"%s\"\n") % name % basic % extended);
}

namespace boost
{

std::string to_string(const ErrorInfo::Message& e)
{
  return toStringHelper("Error message", e.value());
}

std::string to_string(const ErrorInfo::PossibleCauses& e)
{
  if (e.value().size() == 0) {
    return toStringHelper("Possible cause", "<none given>");
  } else if (e.value().size() == 1) {
    return toStringHelper("Possible cause", e.value()[0]);
  } else {
    std::ostringstream oss;
    oss << "[Possible causes]:\n";
    for (const auto& string : e.value()) {
      oss << "  o  " << string << "\n";
    }
    return oss.str();
  }
}

std::string to_string(const ErrorInfo::ReadyFifoStatus& e)
{
  return b::str(b::format("0x%x") % e.value());
}

std::string to_string(const ErrorInfo::PciId& e)
{
  return b::str(b::format("[PCI ID (device, vendor)] = 0x%s 0x%s\n") % e.value().getDeviceId() % e.value().getVendorId());
}

std::string to_string(const ErrorInfo::PciIds& e)
{
  if (e.value().size() == 0) {
    return "";
  } else if (e.value().size() == 1) {
    auto pciId = e.value()[0];
    return b::str(b::format("[PCI IDs (device, vendor)] = 0x%s 0x%s\n") % pciId.getDeviceId() % pciId.getVendorId());
  } else {
    std::ostringstream oss;
    oss << "[PCI IDs (device, vendor)]:\n";
    for (size_t i = 0; i < e.value().size(); ++i) {
      auto pciId = e.value()[i];
      oss << b::str(b::format("  %d. 0x%s 0x%s\n") % i % pciId.getDeviceId() % pciId.getVendorId());
    }
    return oss.str();
  }
}

std::string to_string(const ErrorInfo::DataSource& e)
{
  return toStringHelper("ReadoutCard data source", e.value(), DataSource::toString(e.value()));
}

std::string to_string(const ErrorInfo::ResetLevel& e)
{
  return toStringHelper("ReadoutCard reset level", e.value(), ResetLevel::toString(e.value()));
}

std::string to_string(const ErrorInfo::CardType& e)
{
  return toStringHelper("ReadoutCard card type", e.value(), CardType::toString(e.value()));
}

std::string to_string(const ErrorInfo::PciAddress& e)
{
  return toStringHelper("ReadoutCard PCI address", e.value().toString());
}

std::string to_string(const ErrorInfo::PciSequenceNumber& e)
{
  return toStringHelper("ReadoutCard PCI sequence number", e.value().toString());
}

std::string to_string(const ErrorInfo::SerialId& e)
{
  return toStringHelper("ReadoutCard Serial ID", e.value().toString());
}

std::string to_string(const ErrorInfo::CardId& e)
{
  std::string message;
  if (auto serialIdMaybe = boost::get<SerialId>(&e.value())) {
    auto serialId = *serialIdMaybe;
    message = serialId.toString();
  } else if (auto addressMaybe = boost::get<PciAddress>(&e.value())) {
    auto address = *addressMaybe;
    message = address.toString();
  } else if (auto sequenceNumberMaybe = boost::get<PciSequenceNumber>(&e.value())) {
    auto sequenceNumber = *sequenceNumberMaybe;
    message = sequenceNumber.toString();
  } else {
    message = "unknown";
  }
  return toStringHelper("ReadoutCard Card ID", message);
}

std::string to_string(const ErrorInfo::ConfigParse& e)
{
  return b::str(b::format("Invalid or missing property for group [%s]") % e.value());
}

} // namespace boost
