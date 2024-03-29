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

/// \file PciAddress.cxx
/// \brief Test of the PciAddress class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#define BOOST_TEST_MODULE RORC_TestPciAddress
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"

using namespace ::o2::roc;

namespace
{

// Test string-based constructor
BOOST_AUTO_TEST_CASE(PciAddressTest)
{
  PciAddress address("01:02.3");
  BOOST_REQUIRE(address.getBus() == 1);
  BOOST_REQUIRE(address.getSlot() == 2);
  BOOST_REQUIRE(address.getFunction() == 3);
}

// Test integer-based constructor
BOOST_AUTO_TEST_CASE(PciAddressTest2)
{
  PciAddress address(1, 2, 3);
  BOOST_REQUIRE(address.getBus() == 1);
  BOOST_REQUIRE(address.getSlot() == 2);
  BOOST_REQUIRE(address.getFunction() == 3);
}

// Test formatting
BOOST_AUTO_TEST_CASE(PciAddressTest3)
{
  BOOST_CHECK_THROW(PciAddress("01:02.-3"), ParseException);
  BOOST_CHECK_THROW(PciAddress("01:-2.3"), ParseException);
  BOOST_CHECK_THROW(PciAddress("-1:02.3"), ParseException);
  BOOST_CHECK_THROW(PciAddress("01.02.3"), ParseException);
  BOOST_CHECK_THROW(PciAddress("01:02:3"), ParseException);
  BOOST_CHECK_THROW(PciAddress("01.02:3"), ParseException);
}

// Test ranges
BOOST_AUTO_TEST_CASE(PciAddressTest4)
{
  BOOST_CHECK_THROW(PciAddress(-1, 2, 3), ParameterException);
  BOOST_CHECK_THROW(PciAddress(1, -2, 3), ParameterException);
  BOOST_CHECK_THROW(PciAddress(1, 2, -3), ParameterException);

  BOOST_CHECK_NO_THROW(PciAddress(0xff, 0x1f, 7));
  BOOST_CHECK_THROW(PciAddress(0xff + 1, 0x1f + 0, 7 + 0), ParameterException);
  BOOST_CHECK_THROW(PciAddress(0xff + 0, 0x1f + 1, 7 + 0), ParameterException);
  BOOST_CHECK_THROW(PciAddress(0xff + 0, 0x1f + 0, 7 + 1), ParameterException);
}

// Test string conversion
BOOST_AUTO_TEST_CASE(PciAddressTest5)
{
  auto a = PciAddress("ff:1f.7");
  auto b = PciAddress(a.toString());
  BOOST_REQUIRE(a == b);
}

//  std::vector<std::string> strings { "hello", "1.23", "42" };
//
//  std::string x;
//  double y;
//  int z;
//  BOOST_CHECK_NO_THROW(Util::convertAssign(strings, x, y, z));
//  BOOST_CHECK(x == "hello");
//  BOOST_CHECK(y == 1.23);
//  BOOST_CHECK(z == 42);
//
//  int tooMany;
//  BOOST_CHECK_THROW(Util::convertAssign(strings, x, y, z, tooMany), UtilException);

} // Anonymous namespace
