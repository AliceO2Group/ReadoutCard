/// \file TestProgramOptions.cxx
/// \brief Test of the program options handling
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/RORC.h"
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
  using namespace AliceO2::Rorc::CommandLineUtilities::Options;
  namespace po = boost::program_options;

  // Our mock options
  std::vector<const char*> args = {
      "/test",
      "--address=0x100",
      "--range=200",
      "--value=0x250",
      "--page-size=300",
      "--generator=true",
      "--loopback=RORC",
      "--serial=500",
  };

  // Add option descriptions
  boost::program_options::options_description od;
  addOptionChannel(od);
  addOptionRegisterAddress(od);
  addOptionRegisterValue(od);
  addOptionRegisterRange(od);
  addOptionsChannelParameters(od);
  addOptionSerialNumber(od);

  // Parse options
  po::variables_map vm;
  po::store(po::parse_command_line(args.size(), args.data(), od), vm);
  po::notify(vm);

  // Check results
  BOOST_CHECK_MESSAGE(getOptionRegisterAddress(vm) == 0x100, "register address");
  BOOST_CHECK_MESSAGE(getOptionRegisterRange(vm) == 200, "register range");
  BOOST_CHECK_MESSAGE(getOptionRegisterValue(vm) == 0x250, "register value");
  auto map = getOptionsParameterMap(vm);
  BOOST_CHECK_MESSAGE(map.getDmaPageSizeRequired() == 300l * 1024l, "dma page size");
  BOOST_CHECK_MESSAGE(map.getGeneratorEnabledRequired() == true, "generator enable");
  BOOST_CHECK_MESSAGE(map.getGeneratorLoopbackRequired() == AliceO2::Rorc::LoopbackMode::Rorc, "generator loopback mode");
  BOOST_CHECK_MESSAGE(getOptionSerialNumber(vm) == 500, "serial number");
}

