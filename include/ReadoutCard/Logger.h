
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file Logger.h
/// \brief Definition of InfoLogger functions for ALF
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_LOGGER_H_
#define O2_READOUTCARD_INCLUDE_LOGGER_H_

#include "ReadoutCard/NamespaceAlias.h"
#include "InfoLogger/InfoLogger.hxx"
#include "InfoLogger/InfoLoggerMacros.hxx"

namespace o2
{
namespace roc
{

constexpr auto endm = AliceO2::InfoLogger::InfoLogger::endm;

typedef AliceO2::InfoLogger::InfoLoggerContext ILContext;
typedef AliceO2::InfoLogger::InfoLogger::InfoLoggerMessageOption ILMessageOption;
typedef AliceO2::InfoLogger::InfoLogger::AutoMuteToken ILAutoMuteToken;

class Logger
{
 public:
  static Logger& instance(std::string facility = "ReadoutCard");
  static void setFacility(std::string facility);
  static AliceO2::InfoLogger::InfoLogger& get();

  static void enableInfoLogger(bool state);

 private:
  Logger(std::string facility);              // private, cannot be called
  Logger(Logger const&) = delete;            // copy constructor private
  Logger& operator=(Logger const&) = delete; // assignment operator private
  AliceO2::InfoLogger::InfoLogger mLogger;
  std::string mBaseFacility;
};

} // namespace roc
} // namespace o2

#endif // O2_ALF_LOGGER_H_
