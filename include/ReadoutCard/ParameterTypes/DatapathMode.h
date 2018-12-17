/// \file DatapathMode.h
/// \brief Definition of the DatapathMode enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CRU_DATAPATHMODE_H_
#define ALICEO2_INCLUDE_READOUTCARD_CRU_DATAPATHMODE_H_

#include <string>
#include <Cru/Constants.h>

namespace AliceO2 {
namespace roc {

/// Namespace for the ROC datapath mode enum, and supporting functions
struct DatapathMode
{
    enum type
    {
      Continuous = Cru::Registers::GBT_CONTINUOUS,
      Packet = Cru::Registers::GBT_PACKET,
    };

    /// Converts a DatapathMode to a string
    static std::string toString(const DatapathMode::type& datapathMode);

    /// Converts a string to a DatapathMode
    static DatapathMode::type fromString(const std::string& string);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_DATAPATHMODE_H_
