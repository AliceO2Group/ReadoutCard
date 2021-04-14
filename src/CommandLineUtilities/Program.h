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

#ifndef O2_READOUTCARD_CLI_PROGRAM_H
#define O2_READOUTCARD_CLI_PROGRAM_H

#include <atomic>
#include <cstdlib>
#include <boost/program_options.hpp>
#include "Common/Program.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/Logger.h"

namespace o2
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
  Program(bool ilgEnabled = false)
  {
    enableInfoLogger(ilgEnabled); // Redirect to stdout by default
  }
  virtual ~Program() = default;

 protected:
  std::string getMonitoringUri()
  {
    return MONITORING_URI;
  }

  void enableInfoLogger(bool state)
  {
    // don't interfere if env var explicitly set
    if (std::getenv("O2_INFOLOGGER_MODE")) {
      return;
    }

    Logger::enableInfoLogger(state);
  }

 private:
  const std::string MONITORING_URI = "influxdb-stdout://";
};

} // namespace CommandLineUtilities
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CLI_PROGRAM_H
