///
/// \file RorcException.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "RorcException.h"
#include <stdexcept>
#include <string>
#include <sstream>
#include <boost/exception/all.hpp>
#include <boost/format.hpp>
#include <cstdint>
#include "RORC/ChannelParameters.h"
#include "RORC/CardType.h"
#include "RorcStatusCode.h"

using namespace AliceO2::Rorc;
namespace b = boost;

namespace AliceO2 {
namespace Rorc {

void addPossibleCauses(boost::exception& exception, const std::vector<std::string>& newCauses)
{
  if (auto oldCauses = boost::get_error_info<errinfo_rorc_possible_causes>(exception)) {
    // If we already had some possible causes, we add the new ones to them
    // Note that we also preserve the chronological order, so messages which are closest to the original throw
    // site -- which might even be more likely to reveal the underlying issue -- appear first in the diagnostic output.
    oldCauses->insert(oldCauses->end(), newCauses.begin(), newCauses.end());
  }
  else {
    // Otherwise, we can just them to the exception as usual
    exception << errinfo_rorc_possible_causes(newCauses);
  }
}

} // namespace Rorc
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

std::string to_string(const errinfo_rorc_generic_message& e)
{
  return toStringHelper("Error message", e.value());
}

std::string to_string(const errinfo_rorc_possible_causes& e)
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
    for (auto& string : e.value()) {
      oss << "  o  " << string << "\n";
    }
    return oss.str();
  }
}

std::string to_string(const errinfo_rorc_pci_id& e)
{
  return b::str(b::format("[PCI ID (device, vendor)] = 0x%s 0x%s\n") % e.value().getDeviceId() % e.value().getVendorId());
}

std::string to_string(const errinfo_rorc_loopback_mode& e)
{
  return toStringHelper("RORC loopback mode", e.value(), LoopbackMode::toString(e.value()));
}

std::string to_string(const errinfo_rorc_reset_level& e)
{
  return toStringHelper("RORC reset level", e.value(), ResetLevel::toString(e.value()));
}

std::string to_string(const errinfo_rorc_status_code& e)
{
  return toStringHelper("RORC C API status code", e.value(), getRorcStatusString(e.value()));
}

std::string to_string(const errinfo_rorc_card_type& e)
{
  return toStringHelper("RORC card type", e.value(), CardType::toString(e.value()));
}


} // namespace boost
