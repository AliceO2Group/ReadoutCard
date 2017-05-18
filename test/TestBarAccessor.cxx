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
    // Integrated firmware should have everything
    FirmwareFeatures f = Cru::BarAccessor::convertToFirmwareFeatures(0x40000000);
    BOOST_CHECK(!f.standalone);
    BOOST_CHECK(f.serial);
    BOOST_CHECK(f.loopback0x8000020Bar2Register);
    BOOST_CHECK(f.temperature);
    BOOST_CHECK(f.firmwareInfo);
  }
  {
    // Standalone with everything enabled
    FirmwareFeatures f = Cru::BarAccessor::convertToFirmwareFeatures(0x40005AFE);
    BOOST_CHECK(f.standalone);
    BOOST_CHECK(f.serial);
    BOOST_CHECK(f.loopback0x8000020Bar2Register);
    BOOST_CHECK(f.temperature);
    BOOST_CHECK(f.firmwareInfo);
  }
  {
    // Standalone with everything disabled
    FirmwareFeatures f = Cru::BarAccessor::convertToFirmwareFeatures(0x40005AFE + (0b1111 << 16));
    BOOST_CHECK(f.standalone);
    BOOST_CHECK(!f.serial);
    BOOST_CHECK(!f.loopback0x8000020Bar2Register);
    BOOST_CHECK(!f.temperature);
    BOOST_CHECK(!f.firmwareInfo);
  }
  {
    // Standalone individual features disabled
    BOOST_CHECK(!Cru::BarAccessor::convertToFirmwareFeatures(0x40005AFE + (1 << 16)).loopback0x8000020Bar2Register);
    BOOST_CHECK(!Cru::BarAccessor::convertToFirmwareFeatures(0x40005AFE + (1 << 17)).temperature);
    BOOST_CHECK(!Cru::BarAccessor::convertToFirmwareFeatures(0x40005AFE + (1 << 18)).serial);
    BOOST_CHECK(!Cru::BarAccessor::convertToFirmwareFeatures(0x40005AFE + (1 << 19)).firmwareInfo);
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