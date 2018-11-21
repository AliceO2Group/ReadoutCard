/// \file GbtMux.h
/// \brief Definition of the GbtMux enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMUX_H_
#define ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMUX_H_

#include <string>
#include <Cru/Constants.h>

namespace AliceO2 {
namespace roc {

/// Namespace for the ROC GBT mux enum, and supporting functions
struct GbtMux
{
    enum type
    {
      Ttc = Cru::Registers::GBT_MUX_TTC,
      Ddg = Cru::Registers::GBT_MUX_DDG,
      Sc = Cru::Registers::GBT_MUX_SC,
    };

    /// Converts a GbtMux to an int
    static std::string toString(const GbtMux::type& gbtMux);

    /// Converts a string to a GbtMux
    static GbtMux::type fromString(const std::string& string);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_GBTMUX_H_
