/// \file TestBarAcessor.cxx
/// \brief Tests for CruBar
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos

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
