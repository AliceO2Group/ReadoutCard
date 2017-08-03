/// \file ServiceNames.cxx
/// \brief Implementation of ALICE Lowlevel Frontend (ALF) DIM service names
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ServiceNames.h"
#include <boost/format.hpp>

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Alf {

#define DEFSERVICENAME(_function, _name) \
std::string ServiceNames::_function() const \
{ \
  return format(_name); \
}

DEFSERVICENAME(registerReadRpc, "REGISTER_READ")
DEFSERVICENAME(registerWriteRpc, "REGISTER_WRITE")
DEFSERVICENAME(publishStartCommandRpc, "PUBLISH_SERVICE")
DEFSERVICENAME(publishStopCommandRpc, "PUBLISH_SERVICE_STOP")
DEFSERVICENAME(scaRead, "SCA_READ")
DEFSERVICENAME(scaWrite, "SCA_WRITE")
DEFSERVICENAME(scaGpioWrite, "SCA_GPIO_WRITE")
DEFSERVICENAME(scaGpioRead, "SCA_GPIO_READ")
DEFSERVICENAME(temperature, "TEMPERATURE")

std::string ServiceNames::format(std::string name) const
{
  return ((boost::format("ALF/SERIAL_%d/%s") % serial % name)).str();
}

} // namespace Alf
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
