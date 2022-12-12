
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
/// \file Options.h
/// \brief Definition of functions for the ReadoutCard utilities to handle program options
///
/// The idea is that similar options which appear across multiple utilities, should be handled in a standardized way.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_CLI_OPTIONS_H
#define O2_READOUTCARD_CLI_OPTIONS_H

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/program_options.hpp>
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ParameterTypes/ResetLevel.h"

namespace o2
{
namespace roc
{
namespace CommandLineUtilities
{
namespace Options
{

void addOptionHelp(boost::program_options::options_description& options);
void addOptionRegisterAddress(boost::program_options::options_description& options);
void addOptionRegisterValue(boost::program_options::options_description& options);
void addOptionRegisterRange(boost::program_options::options_description& options);
void addOptionChannel(boost::program_options::options_description& options);
void addOptionResetLevel(boost::program_options::options_description& options);
void addOptionCardId(boost::program_options::options_description& options);

uint32_t getOptionRegisterAddress(const boost::program_options::variables_map& map);
uint32_t getOptionRegisterValue(const boost::program_options::variables_map& map);
int getOptionChannel(const boost::program_options::variables_map& map);
ResetLevel::type getOptionResetLevel(const boost::program_options::variables_map& map);
Parameters::CardIdType getOptionCardId(const boost::program_options::variables_map& map);
std::string getOptionCardIdString(const boost::program_options::variables_map& map);
int getOptionRegisterRange(const boost::program_options::variables_map& map);

} // namespace Options
} // namespace CommandLineUtilities
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CLI_OPTIONS_H
