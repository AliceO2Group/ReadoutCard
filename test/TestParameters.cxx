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
  Parameters p;
  p.put<Parameters::SerialNumber>(SERIAL_NUMBER);
  p.put<Parameters::ChannelNumber>(CHANNEL_NUMBER);
  p.put<Parameters::DmaBufferSize>(DMA_BUFFER_SIZE);
  p.put<Parameters::DmaPageSize>(DMA_PAGE_SIZE);
  p.put<Parameters::GeneratorDataSize>(GENERATOR_DATA_SIZE);
  p.put<Parameters::GeneratorEnabled>(GENERATOR_ENABLED);
  p.put<Parameters::GeneratorLoopbackMode>(GENERATOR_LOOPBACK_MODE);

  BOOST_REQUIRE(p.get<Parameters::SerialNumber>().get_value_or(0) == SERIAL_NUMBER);
  BOOST_REQUIRE(p.get<Parameters::ChannelNumber>().get_value_or(0) == CHANNEL_NUMBER);
  BOOST_REQUIRE(p.get<Parameters::DmaBufferSize>().get_value_or(0) == DMA_BUFFER_SIZE);
  BOOST_REQUIRE(p.get<Parameters::DmaPageSize>().get_value_or(0) == DMA_PAGE_SIZE);
  BOOST_REQUIRE(p.get<Parameters::GeneratorDataSize>().get_value_or(0) == GENERATOR_DATA_SIZE);
  BOOST_REQUIRE(p.get<Parameters::GeneratorEnabled>().get_value_or(false) == GENERATOR_ENABLED);
  BOOST_REQUIRE(p.get<Parameters::GeneratorLoopbackMode>().get_value_or(LoopbackMode::None) == GENERATOR_LOOPBACK_MODE);

  BOOST_REQUIRE(p.getRequired<Parameters::SerialNumber>() == SERIAL_NUMBER);
  BOOST_REQUIRE(p.getRequired<Parameters::ChannelNumber>() == CHANNEL_NUMBER);
  BOOST_REQUIRE(p.getRequired<Parameters::DmaBufferSize>() == DMA_BUFFER_SIZE);
  BOOST_REQUIRE(p.getRequired<Parameters::DmaPageSize>() == DMA_PAGE_SIZE);
  BOOST_REQUIRE(p.getRequired<Parameters::GeneratorDataSize>() == GENERATOR_DATA_SIZE);
  BOOST_REQUIRE(p.getRequired<Parameters::GeneratorEnabled>() == GENERATOR_ENABLED);
  BOOST_REQUIRE(p.getRequired<Parameters::GeneratorLoopbackMode>() == GENERATOR_LOOPBACK_MODE);
}

BOOST_AUTO_TEST_CASE(ParametersMakeTest)
{
  auto p = Parameters::makeParameters(SERIAL_NUMBER, CHANNEL_NUMBER);
  BOOST_REQUIRE(p.getRequired<Parameters::SerialNumber>() == SERIAL_NUMBER);
  BOOST_REQUIRE(p.getRequired<Parameters::ChannelNumber>() == CHANNEL_NUMBER);
}

BOOST_AUTO_TEST_CASE(ParametersThrowTest)
{
  auto p = Parameters::makeParameters(SERIAL_NUMBER, CHANNEL_NUMBER);
  BOOST_CHECK_THROW(p.getRequired<Parameters::GeneratorEnabled>(), ParameterException);
}
