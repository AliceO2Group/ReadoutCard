/// \file ChannelBase.h
/// \brief Definition of the ChannelBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <string>
#include <boost/optional.hpp>
#include <InfoLogger/InfoLogger.hxx>

namespace AliceO2 {
namespace roc {

/// Implements channel functionality common to Master and Slave channels. It provides:
/// - Logging facilities
class ChannelBase
{
  public:
    void setLogLevel(InfoLogger::InfoLogger::Severity severity);

  protected:
    InfoLogger::InfoLogger& getLogger()
    {
      return mLogger;
    }

    InfoLogger::InfoLogger::Severity getLogLevel()
    {
      return mLogLevel;
    }

    void log(int serialNumber, int channelNumber, const std::string& message,
        boost::optional<InfoLogger::InfoLogger::Severity> severity = boost::none);

  private:
    /// InfoLogger instance
    InfoLogger::InfoLogger mLogger;

    /// Current log level
    InfoLogger::InfoLogger::Severity mLogLevel;
};

} // namespace roc
} // namespace AliceO2
