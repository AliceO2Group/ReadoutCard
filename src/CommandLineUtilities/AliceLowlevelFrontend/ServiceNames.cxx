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
DEFSERVICENAME(publishRegistersStart, "PUBLISH_REGISTERS_START")
DEFSERVICENAME(publishRegistersStop, "PUBLISH_REGISTERS_STOP")
DEFSERVICENAME(publishScaSequenceStart, "PUBLISH_SCA_SEQUENCE_START")
DEFSERVICENAME(publishScaSequenceStop, "PUBLISH_SCA_SEQUENCE_STOP")
DEFSERVICENAME(scaRead, "SCA_READ")
DEFSERVICENAME(scaWrite, "SCA_WRITE")
DEFSERVICENAME(scaSequence, "SCA_SEQUENCE")
DEFSERVICENAME(scaGpioWrite, "SCA_GPIO_WRITE")
DEFSERVICENAME(scaGpioRead, "SCA_GPIO_READ")
DEFSERVICENAME(temperature, "TEMPERATURE")

std::string ServiceNames::format(std::string name) const
{
  return ((boost::format("ALF/SERIAL_%1%/LINK_%2%/%3%") % serial % link % name)).str();
}

std::string ServiceNames::publishRegistersSubdir(std::string name) const
{
  return format("PUBLISH_REGISTERS/") + name;
}

std::string ServiceNames::publishScaSequenceSubdir(std::string name) const
{
  return format("PUBLISH_SCA_SEQUENCE/") + name;
}


} // namespace Alf
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
