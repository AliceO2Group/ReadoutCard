/// \file TestBarAcessor.cxx
/// \brief Tests for CruBar
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#define BOOST_TEST_MODULE RORC_TestCruBar
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include "ReadoutCard/BarInterface.h"
#include "Cru/CruBar.h"

using namespace AliceO2::roc;

BOOST_AUTO_TEST_CASE(TestFirmwareFeatures)
{
  {
    // Integrated firmware should have everything
    FirmwareFeatures f = CruBar::convertToFirmwareFeatures(0x40000000);
    BOOST_CHECK(!f.standalone);
    BOOST_CHECK(f.serial);
    BOOST_CHECK(f.dataSelection);
    BOOST_CHECK(f.temperature);
    BOOST_CHECK(f.firmwareInfo);
  }
  {
    // Standalone with everything enabled
    FirmwareFeatures f = CruBar::convertToFirmwareFeatures(0x40005AFE);
    BOOST_CHECK(f.standalone);
    BOOST_CHECK(f.serial);
    BOOST_CHECK(f.dataSelection);
    BOOST_CHECK(f.temperature);
    BOOST_CHECK(f.firmwareInfo);
  }
  {
    // Standalone with everything disabled
    FirmwareFeatures f = CruBar::convertToFirmwareFeatures(0x40005AFE + (0b1111 << 16));
    BOOST_CHECK(f.standalone);
    BOOST_CHECK(!f.serial);
    BOOST_CHECK(!f.dataSelection);
    BOOST_CHECK(!f.temperature);
    BOOST_CHECK(!f.firmwareInfo);
  }
  {
    // Standalone individual features disabled
    BOOST_CHECK(!CruBar::convertToFirmwareFeatures(0x40005AFE + (1 << 16)).dataSelection);
    BOOST_CHECK(!CruBar::convertToFirmwareFeatures(0x40005AFE + (1 << 17)).temperature);
    BOOST_CHECK(!CruBar::convertToFirmwareFeatures(0x40005AFE + (1 << 18)).serial);
    BOOST_CHECK(!CruBar::convertToFirmwareFeatures(0x40005AFE + (1 << 19)).firmwareInfo);
  }
}

BOOST_AUTO_TEST_CASE(TestDataGeneratorConfiguration)
{
  {
    uint32_t bits = 0;
    CruBar::setDataGeneratorEnableBits(bits, true);
    BOOST_CHECK(bits == 0x1);
    CruBar::setDataGeneratorEnableBits(bits, false);
    BOOST_CHECK(bits == 0x0);
  }
  {
    uint32_t bits = 0;
    CruBar::setDataGeneratorRandomSizeBits(bits, true);
    BOOST_CHECK(bits == (1 << 16));
    CruBar::setDataGeneratorRandomSizeBits(bits, false);
    BOOST_CHECK(bits != (1 << 16));
  }
  {
    uint32_t bits = 0;
    CruBar::setDataGeneratorEnableBits(bits, true);
    BOOST_CHECK(bits == 0x1);
    CruBar::setDataGeneratorPatternBits(bits, GeneratorPattern::Incremental);
    BOOST_CHECK(bits == 0x3);
    CruBar::setDataGeneratorSizeBits(bits, 8 * 1024);
    BOOST_CHECK(bits == 0xff03);
  }
  {
    uint32_t bits = 0;
    CruBar::setDataGeneratorEnableBits(bits, true);
    CruBar::setDataGeneratorPatternBits(bits, GeneratorPattern::Incremental);
    CruBar::setDataGeneratorSizeBits(bits, 32);
    BOOST_CHECK(bits == 0x0003);
  }
  {
    uint32_t bits = 0;
    // Too high value
    BOOST_CHECK_THROW(CruBar::setDataGeneratorSizeBits(bits, 8 * 1024 + 1), std::exception);
    // Not a multiple of 256 bits
    BOOST_CHECK_THROW(CruBar::setDataGeneratorSizeBits(bits, 257), std::exception);
  }
}
