/// \file TestParameters.cxx
/// \brief Tests for Parameters
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/Parameters.h"

#define BOOST_TEST_MODULE RORC_TestEnums
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include "RORC/Exception.h"

using namespace AliceO2::Rorc;

constexpr auto SERIAL_NUMBER = 1;
constexpr auto CHANNEL_NUMBER = 2;
constexpr auto DMA_BUFFER_SIZE = 3ul;
constexpr auto DMA_PAGE_SIZE = 4ul;
constexpr auto GENERATOR_DATA_SIZE = 5ul;
constexpr auto GENERATOR_ENABLED = true;
constexpr auto GENERATOR_LOOPBACK_MODE = LoopbackMode::Rorc;

BOOST_AUTO_TEST_CASE(ParametersPutGetTest)
{
  Parameters p = Parameters::makeParameters(SERIAL_NUMBER, CHANNEL_NUMBER)
      .setDmaBufferSize(DMA_BUFFER_SIZE)
      .setDmaPageSize(DMA_PAGE_SIZE)
      .setGeneratorDataSize(GENERATOR_DATA_SIZE)
      .setGeneratorEnabled(GENERATOR_ENABLED)
      .setGeneratorLoopback(GENERATOR_LOOPBACK_MODE)
      .setBufferParameters(BufferParameters::File{"/my/file.shm", 0});

  BOOST_REQUIRE(boost::get<int>(p.getCardId().get()) == SERIAL_NUMBER);
  BOOST_REQUIRE(p.getChannelNumber().get_value_or(0) == CHANNEL_NUMBER);
  BOOST_REQUIRE(p.getDmaBufferSize().get_value_or(0) == DMA_BUFFER_SIZE);
  BOOST_REQUIRE(p.getDmaPageSize().get_value_or(0) == DMA_PAGE_SIZE);
  BOOST_REQUIRE(p.getGeneratorDataSize().get_value_or(0) == GENERATOR_DATA_SIZE);
  BOOST_REQUIRE(p.getGeneratorEnabled().get_value_or(false) == GENERATOR_ENABLED);
  BOOST_REQUIRE(p.getGeneratorLoopback().get_value_or(LoopbackMode::None) == GENERATOR_LOOPBACK_MODE);

  BOOST_REQUIRE(boost::get<int>(p.getCardIdRequired()) == SERIAL_NUMBER);
  BOOST_REQUIRE(p.getChannelNumberRequired() == CHANNEL_NUMBER);
  BOOST_REQUIRE(p.getDmaBufferSizeRequired() == DMA_BUFFER_SIZE);
  BOOST_REQUIRE(p.getDmaPageSizeRequired() == DMA_PAGE_SIZE);
  BOOST_REQUIRE(p.getGeneratorDataSizeRequired() == GENERATOR_DATA_SIZE);
  BOOST_REQUIRE(p.getGeneratorEnabledRequired() == GENERATOR_ENABLED);
  BOOST_REQUIRE(p.getGeneratorLoopbackRequired() == GENERATOR_LOOPBACK_MODE);

  BOOST_REQUIRE(boost::get<BufferParameters::File>(p.getBufferParametersRequired()).path == "/my/file.shm");
  BOOST_REQUIRE(boost::get<BufferParameters::File>(p.getBufferParametersRequired()).size == 0);
}

BOOST_AUTO_TEST_CASE(ParametersThrowTest)
{
  auto p = Parameters::makeParameters(SERIAL_NUMBER, CHANNEL_NUMBER);
  BOOST_CHECK_THROW(p.getGeneratorEnabledRequired(), ParameterException);
}
