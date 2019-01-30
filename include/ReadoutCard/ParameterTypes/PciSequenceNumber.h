/// \file PciSequenceNumber.h
/// \brief Definition of the PciSequenceNumber class.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_INCLUDE_PCISEQUENCENUMBER_H_
#define ALICEO2_READOUTCARD_INCLUDE_PCISEQUENCENUMBER_H_

#include <string>
#include <boost/optional.hpp>

namespace AliceO2 {
namespace roc {

/// Data holder class for a PCI Sequence Number, a string starting with # (e.g. #01, #4)
class PciSequenceNumber
{
  public:
    /// Constructs a PciSequenceNumber object using a string in the #xxx format
    /// For example: "#01", "#4", "#0"
    /// \param string String of format "^#[0-9]+$"
    PciSequenceNumber(const std::string& string);

    /// Converts to a PciSequenceNumber object from a string that matches "^#[0-9]+$"
    /// For example: "#04"
    static boost::optional<PciSequenceNumber> fromString(std::string string);

    bool operator==(const int& other) const
    {
      return mSequenceNumber == other;
    }

    bool operator==(const PciSequenceNumber& other) const
    {
      return mSequenceNumber == other.mSequenceNumber;
    }
 
  private:
    int mSequenceNumber;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_INCLUDE_READOUTCARD_H_
