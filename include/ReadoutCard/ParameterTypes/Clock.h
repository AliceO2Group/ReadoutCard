/// \file Clock.h
/// \brief Definition of the Clock enum and supporting functions.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CLOCK_H_
#define ALICEO2_INCLUDE_READOUTCARD_CLOCK_H_

#include <string>
#include <Cru/Constants.h>

namespace AliceO2 {
namespace roc {

/// Namespace for the ROC clock enum, and supporting functions
struct Clock
{
    enum type
    {
      Local = Cru::Registers::CLOCK_LOCAL,
      Ttc = Cru::Registers::CLOCK_TTC,
    };

    /// Converts a Clock to a string
    static std::string toString(const Clock::type& clock);

    /// Converts a string to a Clock
    static Clock::type fromString(const std::string& string);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_CLOCK_H_
