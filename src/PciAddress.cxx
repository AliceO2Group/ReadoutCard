/// \file PciAddress.h
/// \brief Implementation of the PciAddress struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/PciAddress.h"
#include <iomanip>
#include <sstream>
#include <boost/spirit/include/qi.hpp>
#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {

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
          << errinfo_rorc_error_message("Bus number out of range")
          << errinfo_rorc_pci_address_bus_number(bus));
    }

    if (slot < 0 || slot > 0x1f) {
      BOOST_THROW_EXCEPTION(ParameterException()
          << errinfo_rorc_error_message("Slot number out of range")
          << errinfo_rorc_pci_address_slot_number(slot));
    }

    if (function < 0 || function > 7) {
      BOOST_THROW_EXCEPTION(ParameterException()
          << errinfo_rorc_error_message("Function number out of range")
          << errinfo_rorc_pci_address_function_number(function));
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
        << errinfo_rorc_error_message("Parsing PCI address failed"));
  }
  checkRanges(bus, slot, function);
}

std::string PciAddress::toString() const
{
  std::ostringstream oss;
  oss << std::hex << bus << ':' << slot << '.' << function;
  return oss.str();
}

} // namespace Rorc
} // namespace AliceO2
