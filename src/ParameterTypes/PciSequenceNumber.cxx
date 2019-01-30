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
    std::regex expression ("^#[0-9]+$");
    if (std::regex_search(string, expression)) {
      std::string retString = string;
      return stoi(retString.erase(0, 1)); //remove initial #
    } else {
      BOOST_THROW_EXCEPTION(ParseException()
          << ErrorInfo::Message("Parsing PCI sequence number failed"));
    }
  }

}

PciSequenceNumber::PciSequenceNumber(const std::string& string)
{
  mSequenceNumber = parseSequenceNumberString(string);
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

} // namespace roc
} // namespace AliceO2
