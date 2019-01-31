/// \file PciAddress.h
/// \brief Implementation of the PciAddress struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include <iomanip>
#include <sstream>
#include <boost/format.hpp>
#include <boost/spirit/include/qi.hpp>
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace roc {

namespace {
  bool parseLspciFormat(const std::string& string, int& bus, int& slot, int& function)
  {
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
  }
  catch (const ParseException& e) {
    return {};
  }
}

std::ostream& operator<<(std::ostream& os, const PciAddress& pciAddress)
{
  os << pciAddress.toString();
  return os;
}

} // namespace roc
} // namespace AliceO2
