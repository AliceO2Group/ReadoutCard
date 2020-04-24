// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Program.h
/// \brief Definition of the Program class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_PROGRAM_H
#define ALICEO2_READOUTCARD_PROGRAM_H

#include <atomic>
#include <boost/program_options.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include "Common/Program.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "ReadoutCard/Exception.h"

namespace AliceO2
{
namespace roc
{
namespace CommandLineUtilities
{

/// Helper class for making a ReadoutCard utility program. It adds logging facilities to the Common::Program
/// - SIGINT signals
class Program : public AliceO2::Common::Program
{
 public:
  virtual ~Program() = default;

 protected:
  /// Get Program's InfoLogger instance
  InfoLogger::InfoLogger& getLogger()
  {
    return mLogger;
  }

  InfoLogger::InfoLogger::Severity getLogLevel() const
  {
    return mLogLevel;
  }

  void setLogLevel(InfoLogger::InfoLogger::Severity logLevel = InfoLogger::InfoLogger::Severity::Info)
  {
    mLogLevel = logLevel;
  }

  std::string getMonitoringUri()
  {
    return MONITORING_URI;
  }

 private:
  InfoLogger::InfoLogger mLogger;
  InfoLogger::InfoLogger::Severity mLogLevel = InfoLogger::InfoLogger::Severity::Info;

  const std::string MONITORING_URI = "stdout://";
};

} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_PROGRAM_H
