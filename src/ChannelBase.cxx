/// \file ChannelBase.cxx
/// \brief Implementation of the ChannelBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelBase.h"

namespace AliceO2 {
namespace Rorc {

void ChannelBase::setLogLevel(InfoLogger::InfoLogger::Severity severity)
{
  mLogLevel = severity;
}

void ChannelBase::log(int serialNumber, int channelNumber, const std::string& message,
    boost::optional<InfoLogger::InfoLogger::Severity> severity)
{
  mLogger << severity.get_value_or(mLogLevel);
  mLogger << "[serial:" << serialNumber << " channel:" << channelNumber << "] ";
  mLogger << message;
  mLogger << InfoLogger::InfoLogger::endm;
}

} // namespace Rorc
} // namespace AliceO2
