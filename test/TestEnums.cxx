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

/// \file TestEnums.cxx
/// \brief Tests for enums
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/CardType.h"
#include "ReadoutCard/ParameterTypes/DataSource.h"
#include "ReadoutCard/ParameterTypes/ResetLevel.h"

#define BOOST_TEST_MODULE RORC_TestEnums
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

using namespace o2::roc;

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
  checkEnumConversion<CardType>({ CardType::Crorc, CardType::Cru, CardType::Unknown });
}

BOOST_AUTO_TEST_CASE(EnumDataSourceConversion)
{
  checkEnumConversion<DataSource>({ DataSource::Diu, DataSource::Fee, DataSource::Internal, DataSource::Siu });
}

BOOST_AUTO_TEST_CASE(EnumResetLevelConversion)
{
  checkEnumConversion<ResetLevel>({ ResetLevel::Nothing, ResetLevel::Internal, ResetLevel::InternalSiu });
}
