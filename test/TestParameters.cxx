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

using namespace AliceO2::roc;

constexpr auto SERIAL_NUMBER = 1;
constexpr auto CHANNEL_NUMBER = 2;
constexpr auto DMA_BUFFER_SIZE = 3ul;
constexpr auto DMA_PAGE_SIZE = 4ul;
constexpr auto GENERATOR_DATA_SIZE = 5ul;
constexpr auto GENERATOR_ENABLED = true;
constexpr auto GENERATOR_LOOPBACK_MODE = LoopbackMode::Internal;

BOOST_AUTO_TEST_CASE(ParametersConstructors)
{
  Parameters p;
  Parameters p2 = p;
  Parameters p3 = std::move(p2);
  Parameters p4 = Parameters::makeParameters(SERIAL_NUMBER, CHANNEL_NUMBER);
}

BOOST_AUTO_TEST_CASE(ParametersPutGetTest)
{
  Parameters p = Parameters::makeParameters(SERIAL_NUMBER, CHANNEL_NUMBER)
      .setDmaPageSize(DMA_PAGE_SIZE)
      .setGeneratorDataSize(GENERATOR_DATA_SIZE)
      .setGeneratorEnabled(GENERATOR_ENABLED)
      .setGeneratorLoopback(GENERATOR_LOOPBACK_MODE)
      .setBufferParameters(buffer_parameters::File{"/my/file.shm", 0});

  BOOST_REQUIRE(boost::get<int>(p.getCardId().get()) == SERIAL_NUMBER);
  BOOST_REQUIRE(p.getChannelNumber().get_value_or(0) == CHANNEL_NUMBER);
  BOOST_REQUIRE(p.getDmaPageSize().get_value_or(0) == DMA_PAGE_SIZE);
  BOOST_REQUIRE(p.getGeneratorDataSize().get_value_or(0) == GENERATOR_DATA_SIZE);
  BOOST_REQUIRE(p.getGeneratorEnabled().get_value_or(false) == GENERATOR_ENABLED);
  BOOST_REQUIRE(p.getGeneratorLoopback().get_value_or(LoopbackMode::None) == GENERATOR_LOOPBACK_MODE);

  BOOST_REQUIRE(boost::get<int>(p.getCardIdRequired()) == SERIAL_NUMBER);
  BOOST_REQUIRE(p.getChannelNumberRequired() == CHANNEL_NUMBER);
  BOOST_REQUIRE(p.getDmaPageSizeRequired() == DMA_PAGE_SIZE);
  BOOST_REQUIRE(p.getGeneratorDataSizeRequired() == GENERATOR_DATA_SIZE);
  BOOST_REQUIRE(p.getGeneratorEnabledRequired() == GENERATOR_ENABLED);
  BOOST_REQUIRE(p.getGeneratorLoopbackRequired() == GENERATOR_LOOPBACK_MODE);

  BOOST_REQUIRE(boost::get<buffer_parameters::File>(p.getBufferParametersRequired()).path == "/my/file.shm");
  BOOST_REQUIRE(boost::get<buffer_parameters::File>(p.getBufferParametersRequired()).size == 0);
}

BOOST_AUTO_TEST_CASE(ParametersThrowTest)
{
  auto p = Parameters::makeParameters(SERIAL_NUMBER, CHANNEL_NUMBER);
  BOOST_CHECK_THROW(p.getGeneratorEnabledRequired(), ParameterException);
}

BOOST_AUTO_TEST_CASE(ParametersLinkMaskFromString)
{
  {
    auto a = Parameters::LinkMaskType{0, 1, 2, 3, 4, 5};
    BOOST_CHECK(Parameters::linkMaskFromString("0,1,2,3,4,5") == a);
    BOOST_CHECK(Parameters::linkMaskFromString("0-5") == a);
  }
  {
    auto b = Parameters::LinkMaskType{0, 1, 4, 5, 6};
    BOOST_CHECK(Parameters::linkMaskFromString("0,1,4,5,6") == b);
    BOOST_CHECK(Parameters::linkMaskFromString("0,1,4-6") == b);
    BOOST_CHECK(Parameters::linkMaskFromString("0-1,4-6") == b);
  }
}
