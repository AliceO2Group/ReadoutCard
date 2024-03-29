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

/// \file TestParameters.cxx
/// \brief Tests for Parameters
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/Parameters.h"

#define BOOST_TEST_MODULE RORC_TestParameters
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include "ReadoutCard/Exception.h"

using namespace o2::roc;

constexpr auto SERIAL_NUMBER = 10000;
constexpr auto ENDPOINT_NUMBER = 0;
constexpr auto CHANNEL_NUMBER = 2;
//constexpr auto DMA_BUFFER_SIZE = 3ul;
constexpr auto DMA_PAGE_SIZE = 4ul;
constexpr auto DATA_SOURCE = DataSource::Internal;

BOOST_AUTO_TEST_CASE(ParametersConstructors)
{
  Parameters p;
  Parameters p2 = p;
  Parameters p3 = std::move(p2);
  Parameters p4 = Parameters::makeParameters(SerialId{ SERIAL_NUMBER, ENDPOINT_NUMBER }, CHANNEL_NUMBER); //TODO: Check makeParameters
}

BOOST_AUTO_TEST_CASE(ParametersPutGetTest)
{
  Parameters p = Parameters::makeParameters(SerialId{ SERIAL_NUMBER, ENDPOINT_NUMBER }, CHANNEL_NUMBER)
                   .setDmaPageSize(DMA_PAGE_SIZE)
                   .setDataSource(DATA_SOURCE)
                   .setBufferParameters(buffer_parameters::File{ "/my/file.shm", 0 });

  BOOST_REQUIRE(boost::get<SerialId>(p.getCardId().get()) == (SerialId{ SERIAL_NUMBER, ENDPOINT_NUMBER }));
  BOOST_REQUIRE(p.getChannelNumber().get_value_or(0) == CHANNEL_NUMBER);
  BOOST_REQUIRE(p.getDmaPageSize().get_value_or(0) == DMA_PAGE_SIZE);
  BOOST_REQUIRE(p.getDataSource().get_value_or(DataSource::Fee) == DATA_SOURCE);

  BOOST_REQUIRE(boost::get<SerialId>(p.getCardIdRequired()) == (SerialId{ SERIAL_NUMBER, ENDPOINT_NUMBER }));
  BOOST_REQUIRE(p.getChannelNumberRequired() == CHANNEL_NUMBER);
  BOOST_REQUIRE(p.getDmaPageSizeRequired() == DMA_PAGE_SIZE);
  BOOST_REQUIRE(p.getDataSourceRequired() == DATA_SOURCE);

  BOOST_REQUIRE(boost::get<buffer_parameters::File>(p.getBufferParametersRequired()).path == "/my/file.shm");
  BOOST_REQUIRE(boost::get<buffer_parameters::File>(p.getBufferParametersRequired()).size == 0);
}

BOOST_AUTO_TEST_CASE(ParametersThrowTest)
{
  auto p = Parameters::makeParameters(SerialId{ SERIAL_NUMBER, ENDPOINT_NUMBER }, CHANNEL_NUMBER);
}

BOOST_AUTO_TEST_CASE(ParametersLinkMaskFromString)
{
  {
    auto a = Parameters::LinkMaskType{ 0, 1, 2, 3, 4, 5 };
    BOOST_CHECK(Parameters::linkMaskFromString("0,1,2,3,4,5") == a);
    BOOST_CHECK(Parameters::linkMaskFromString("0-5") == a);
  }
  {
    auto b = Parameters::LinkMaskType{ 0, 1, 4, 5, 6 };
    BOOST_CHECK(Parameters::linkMaskFromString("0,1,4,5,6") == b);
    BOOST_CHECK(Parameters::linkMaskFromString("0,1,4-6") == b);
    BOOST_CHECK(Parameters::linkMaskFromString("0-1,4-6") == b);
  }
  {
    auto c = Parameters::LinkMaskType{ 0, 3, 4, 5 };
    BOOST_CHECK(Parameters::linkMaskFromString("0,3-5") == c);
  }
  BOOST_CHECK_THROW(Parameters::linkMaskFromString("0/2/3/4"), ParseException);
  BOOST_CHECK_THROW(Parameters::linkMaskFromString("0,1,2,3+4"), ParseException);
}

BOOST_AUTO_TEST_CASE(ParametersCardIdFromString)
{
  {
    Parameters::CardIdType cardId = PciAddress("42:0.0");
    BOOST_CHECK(Parameters::cardIdFromString("42:0.0") == cardId);
    BOOST_CHECK(Parameters::cardIdFromString("12345") != cardId);
  }
  {
    Parameters::CardIdType cardId = SerialId(12345);
    BOOST_CHECK(Parameters::cardIdFromString("42:0.0") != cardId);
    BOOST_CHECK(Parameters::cardIdFromString("12345") == cardId);
  }
  BOOST_CHECK_THROW(Parameters::cardIdFromString("9123:132745.796"), ParameterException);
  BOOST_CHECK_THROW(Parameters::cardIdFromString("42:0:0"), ParseException);
  BOOST_CHECK_THROW(Parameters::cardIdFromString("3248758792345"), ParseException);
}
