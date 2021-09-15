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

/// \file TestCruDataFormat.cxx
/// \brief Tests for CruDataFormat
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#define BOOST_TEST_MODULE RORC_TestCruDataFormat
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "DataFormat.h"

using namespace o2::roc;
using namespace DataFormat;

static const std::vector<uint32_t> link18Test1 = {
  0xabcd,
  0x0,
  0x212d,
  0xead01001,
  0x0,
  0x0,
  0xda1e5afe,
  0x0,
  0x0,
  0x0,
  0x111caf,
  0xebeef102,
  0x0,
  0x0,
  0x12345,
  0x67810000,
};

static const std::vector<uint32_t> link18Test2 = {
  0x85,
  0x0,
  0x212d,
  0xead01001,
  0x0,
  0x0,
  0xda1e5afe,
  0x3fb,
  0x0,
  0x0,
  0x111caf,
  0xebeefbb7,
  0x0,
  0x0,
  0x12345,
  0x67810000,
};

static const std::vector<uint32_t> link21Test1 = {
  0x0,
  0x0,
  0x215d,
  0xead01001,
  0x0,
  0x0,
  0xda1e5afe,
  0x0,
  0x0,
  0x0,
  0x111caf,
  0xebeef405,
  0x0,
  0x0,
  0x12345,
  0x67810000,
};

BOOST_AUTO_TEST_CASE(TestGetLinkId)
{
  BOOST_CHECK_EQUAL(getLinkId(reinterpret_cast<const char*>(link18Test1.data())), 18);
  BOOST_CHECK_EQUAL(getLinkId(reinterpret_cast<const char*>(link18Test2.data())), 18);
  BOOST_CHECK_EQUAL(getLinkId(reinterpret_cast<const char*>(link21Test1.data())), 21);
}

BOOST_AUTO_TEST_CASE(TestGetEventSize)
{
  BOOST_CHECK_EQUAL(getMemsize(reinterpret_cast<const char*>(link18Test1.data())), 256);
  BOOST_CHECK_EQUAL(getMemsize(reinterpret_cast<const char*>(link18Test2.data())), 256);
  BOOST_CHECK_EQUAL(getMemsize(reinterpret_cast<const char*>(link21Test1.data())), 256);
}
