/// \file GbtMode.h
/// \brief Definition of the GbtMode enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMODE_H_
#define ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMODE_H_

#include <string>
#include <Cru/Constants.h>

namespace AliceO2 {
namespace roc {

/// Namespace for the ROC GBT mux enum, and supporting functions
struct GbtMode
{
    enum type
    {
      Gbt = Cru::Registers::GBT_MODE_GBT,
      Wb = Cru::Registers::GBT_MODE_WB,
    };

    /// Converts a GbtMode to an int
    static std::string toString(const GbtMode::type& gbtMux);

    /// Converts a string to a GbtMode
    static GbtMode::type fromString(const std::string& string);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMODE_H_
