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

/// \file TestProgramOptions.cxx
/// \brief Test of the program options handling
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Options.h"

#define BOOST_TEST_MODULE RORC_TestProgramOptions
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

/// Test handling of program options in the utilities
BOOST_AUTO_TEST_CASE(UtilOptions)
{
  using namespace o2::roc::CommandLineUtilities::Options;
  namespace po = boost::program_options;

  // Our mock options
  std::vector<const char*> args = {
    "/test",
    "--channel=0",
    "--address=0x100",
    "--value=0x250",
    "--range=200",
  };

  // Add option descriptions
  boost::program_options::options_description od;
  addOptionChannel(od);
  addOptionRegisterAddress(od);
  addOptionRegisterValue(od);
  addOptionRegisterRange(od);

  // Parse options
  po::variables_map vm;
  po::store(po::parse_command_line(args.size(), args.data(), od), vm);
  po::notify(vm);

  // Check results
  BOOST_CHECK_MESSAGE(getOptionChannel(vm) == 0, "channel");
  BOOST_CHECK_MESSAGE(getOptionRegisterAddress(vm) == 0x100, "register address");
  BOOST_CHECK_MESSAGE(getOptionRegisterRange(vm) == 200, "register range");
  BOOST_CHECK_MESSAGE(getOptionRegisterValue(vm) == 0x250, "register value");
}
