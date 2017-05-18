/// \file TestBarAcessor.cxx
/// \brief Tests for BarAccessor
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#define BOOST_TEST_MODULE RORC_TestBarAccessor
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include "ReadoutCard/BarInterface.h"
#include "Cru/BarAccessor.h"

using namespace AliceO2::roc;

BOOST_AUTO_TEST_CASE(TestFirmwareFeatures)
{
  {
    FirmwareFeatures f = Cru::BarAccessor::convertToFirmwareFeatures(0x40000000);
    BOOST_CHECK(f.standalone == false);
    BOOST_CHECK(f.serial == true);
    BOOST_CHECK(f.loopback0x8000020Bar2Register == true);
    BOOST_CHECK(f.temperature == true);
  }
  {
    FirmwareFeatures f = Cru::BarAccessor::convertToFirmwareFeatures(0x40015AFE);
    BOOST_CHECK(f.standalone == true);
    BOOST_CHECK(f.serial == false);
    BOOST_CHECK(f.loopback0x8000020Bar2Register == false);
    BOOST_CHECK(f.temperature == true);
  }
  {
    FirmwareFeatures f = Cru::BarAccessor::convertToFirmwareFeatures(0x40035AFE);
    BOOST_CHECK(f.standalone == true);
    BOOST_CHECK(f.serial == false);
    BOOST_CHECK(f.loopback0x8000020Bar2Register == false);
    BOOST_CHECK(f.temperature == false);
  }
}

BOOST_AUTO_TEST_CASE(TestDataGeneratorConfiguration)
{
  {
    uint32_t bits = 0;
    Cru::BarAccessor::setDataGeneratorEnableBits(bits, true);
    BOOST_CHECK(bits == 0x1);
    Cru::BarAccessor::setDataGeneratorEnableBits(bits, false);
    BOOST_CHECK(bits == 0x0);
  }
  {
    uint32_t bits = 0;
    Cru::BarAccessor::setDataGeneratorEnableBits(bits, true);
    BOOST_CHECK(bits == 0x1);
    Cru::BarAccessor::setDataGeneratorPatternBits(bits, GeneratorPattern::Incremental);
    BOOST_CHECK(bits == 0x3);
    Cru::BarAccessor::setDataGeneratorSizeBits(bits, 8*1024);
    BOOST_CHECK(bits == 0xff03);
  }
  {
    uint32_t bits = 0;
    Cru::BarAccessor::setDataGeneratorEnableBits(bits, true);
    Cru::BarAccessor::setDataGeneratorPatternBits(bits, GeneratorPattern::Incremental);
    Cru::BarAccessor::setDataGeneratorSizeBits(bits, 32);
    BOOST_CHECK(bits == 0x0003);
  }
  {
    uint32_t bits = 0;
    // Too high value
    BOOST_CHECK_THROW(Cru::BarAccessor::setDataGeneratorSizeBits(bits, 8*1024 + 1), std::exception);
    // Not a multiple of 256 bits
    BOOST_CHECK_THROW(Cru::BarAccessor::setDataGeneratorSizeBits(bits, 257), std::exception);
  }
}