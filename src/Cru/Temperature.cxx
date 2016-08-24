/// \file Temperature.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Implementation of functions relating to the CRU's temperature sensor

#include "Cru/Temperature.h"

namespace AliceO2 {
namespace Rorc {
namespace Cru {
namespace Temperature {

/// It's a 10 bit register, so: 2^10 - 1
constexpr int REGISTER_MAX_VALUE = 1023;

/// Converts a value from the CRU's temperature register and converts it to a °C double value.
/// \param registerValue Value of the temperature register to convert
/// \return Temperature value in °C or nothing if the registerValue was invalid
boost::optional<double> convertRegisterValue(int32_t registerValue)
{
  // Conversion formula from: https://documentation.altera.com/#/00045071-AA$AA00044865
  if (registerValue == 0 || registerValue > REGISTER_MAX_VALUE) {
    return boost::none;
  } else {
    double A = 693.0;
    double B = 265.0;
    double C = double(registerValue);
    return ((A * C) / 1024.0) - B;
  }
}

} // namespace Temperature
} // namespace Cru
} // namespace Rorc
} // namespace AliceO2
