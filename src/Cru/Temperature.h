/// \file Temperature.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Definition of functions relating to the CRU's temperature sensor

#ifndef SRC_CRU_TEMPERATURE_H_
#define SRC_CRU_TEMPERATURE_H_

#include <boost/optional.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Cru {
namespace Temperature {

/// Maximum recommended operating temperature in °C
constexpr double MAX_TEMPERATURE = 45.0;

/// Converts a value from the CRU's temperature register and converts it to a °C double value.
/// \param registerValue Value of the temperature register
/// \return Temperature value or nothing if the registerValue was invalid
boost::optional<double> convertRegisterValue(int32_t registerValue);

} // namespace Temperature
} // namespace Cru
} // namespace Rorc
} // namespace AliceO2

#endif // SRC_CRU_TEMPERATURE_H_
