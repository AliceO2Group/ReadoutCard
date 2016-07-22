///
/// \file TestRorc.cxx
/// \author Pascal Boeschoten
///

#include <RORC/RORC.h>
#include "../src/RorcUtilsOptions.h"

#define BOOST_TEST_MODULE RORC_Test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

/// Test handling of program options in the utilities
BOOST_AUTO_TEST_CASE(UtilOptions)
{
  using namespace AliceO2::Rorc::Util::Options;
  namespace po = boost::program_options;

  // Our mock options
  std::vector<const char*> args = {
      "/test",
      "--address=0x100",
      "--regrange=200",
      "--value=0x250",
      "--cp-dma-pagesize=300",
      "--cp-dma-bufmb=400",
      "--cp-gen-enable=true",
      "--cp-gen-loopb=RORC",
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
  auto cps = getOptionsChannelParameters(vm);
  BOOST_CHECK_MESSAGE(cps.dma.pageSize == 300, "dma page size");
  BOOST_CHECK_MESSAGE(cps.dma.bufferSize == (400l * 1024l * 1024l), "dma buffer size");
  BOOST_CHECK_MESSAGE(cps.generator.useDataGenerator == true, "generator enable");
  BOOST_CHECK_MESSAGE(cps.generator.loopbackMode == AliceO2::Rorc::LoopbackMode::RORC, "generator loopback mode");
  BOOST_CHECK_MESSAGE(getOptionSerialNumber(vm) == 500, "serial number");
}

/// Helper method for checking enum to/from string conversions
template <typename E>
void checkEnumConversion(const std::vector<typename E::type>& items)
{
  for (auto& item : items) {
    BOOST_CHECK(E::fromString(E::toString(item)) == item);
  }
}

BOOST_AUTO_TEST_CASE(EnumCardTypeConversion)
{
  using namespace AliceO2::Rorc;
  checkEnumConversion<CardType>({CardType::CRORC, CardType::CRU, CardType::DUMMY, CardType::UNKNOWN});
}

BOOST_AUTO_TEST_CASE(EnumLoopbackModeConversion)
{
  using namespace AliceO2::Rorc;
  checkEnumConversion<LoopbackMode>({LoopbackMode::DIU, LoopbackMode::NONE, LoopbackMode::RORC, LoopbackMode::SIU});
}

BOOST_AUTO_TEST_CASE(EnumResetLevelConversion)
{
  using namespace AliceO2::Rorc;
  checkEnumConversion<ResetLevel>({ResetLevel::NOTHING, ResetLevel::RORC, ResetLevel::RORC_DIU,
      ResetLevel::RORC_DIU_SIU});
}
