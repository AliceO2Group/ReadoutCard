/// \file ExceptionInternal.cxx
/// \brief Implementation of internal RORC exceptions and related functions.
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
#include "RorcStatusCode.h"

using namespace AliceO2::roc;
namespace b = boost;

namespace AliceO2 {
namespace roc {

void addPossibleCauses(boost::exception& exception, const std::vector<std::string>& newCauses)
{
  if (auto oldCauses = boost::get_error_info<ErrorInfo::PossibleCauses>(exception)) {
    // If we already had some possible causes, we add the new ones to them
    // Note that we also preserve the chronological order, so messages which are closest to the original throw
    // site -- which might even be more likely to reveal the underlying issue -- appear first in the diagnostic output.
    oldCauses->insert(oldCauses->end(), newCauses.begin(), newCauses.end());
  }
  else {
    // Otherwise, we can just them to the exception as usual
    exception << ErrorInfo::PossibleCauses(newCauses);
  }
}

} // namespace roc
} // namespace AliceO2

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

namespace boost {

std::string to_string(const ErrorInfo::Message& e)
{
  return toStringHelper("Error message", e.value());
}

std::string to_string(const ErrorInfo::PossibleCauses& e)
{
  if (e.value().size() == 0) {
    return toStringHelper("Possible cause", "<none given>");
  }
  else if (e.value().size() == 1) {
    return toStringHelper("Possible cause", e.value()[0]);
  }
  else {
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
  }
  else if (e.value().size() == 1) {
    auto pciId = e.value()[0];
    return b::str(b::format("[PCI IDs (device, vendor)] = 0x%s 0x%s\n") % pciId.getDeviceId() % pciId.getVendorId());
  }
  else {
    std::ostringstream oss;
    oss << "[PCI IDs (device, vendor)]:\n";
    for (size_t i = 0; i < e.value().size(); ++i) {
      auto pciId = e.value()[i];
      oss << b::str(b::format("  %d. 0x%s 0x%s\n") % i % pciId.getDeviceId() % pciId.getVendorId());
    }
    return oss.str();
  }
}

std::string to_string(const ErrorInfo::LoopbackMode& e)
{
  return toStringHelper("RORC loopback mode", e.value(), LoopbackMode::toString(e.value()));
}

std::string to_string(const ErrorInfo::ResetLevel& e)
{
  return toStringHelper("RORC reset level", e.value(), ResetLevel::toString(e.value()));
}

std::string to_string(const ErrorInfo::StatusCode& e)
{
  return toStringHelper("RORC C API status code", e.value(), StatusCode::getString(e.value()));
}

std::string to_string(const ErrorInfo::CardType& e)
{
  return toStringHelper("RORC card type", e.value(), CardType::toString(e.value()));
}

std::string to_string(const ErrorInfo::PciAddress& e)
{
  return toStringHelper("RORC PCI address", e.value().toString());
}

} // namespace boost
