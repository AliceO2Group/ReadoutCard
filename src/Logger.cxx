// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Logger.h
/// \brief Definition of InfoLogger functions for ALF
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "ReadoutCard/Logger.h"

namespace o2
{
namespace roc
{

Logger::Logger(std::string facility)
{
  ILContext context;
  context.setField(ILContext::FieldName::System, "FLP");
  context.setField(ILContext::FieldName::Facility, facility);
  mLogger.setContext(context);
  mLogger << "New " << facility << " InfoLogger connection" << LogDebugTrace << endm;
}

void Logger::setFacility(std::string facility)
{
  ILContext context;
  context.setField(ILContext::FieldName::System, "FLP");
  context.setField(ILContext::FieldName::Facility, facility);
  Logger::instance(facility).mLogger.setContext(context);
  // may flood infologger
  //Logger::get() << "Facility set: " << facility << LogDebugTrace << endm;
}

Logger& Logger::instance(std::string facility)
{
  static Logger instance(facility);
  return instance;
}

AliceO2::InfoLogger::InfoLogger& Logger::get()
{
  return Logger::instance().mLogger;
}

void Logger::enableInfoLogger(bool state)
{
  if (state) {
    setenv("INFOLOGGER_MODE", "infoLoggerD", true);
  } else {
    setenv("INFOLOGGER_MODE", "stdout", true);
  }
}

} // namespace roc
} // namespace o2
