/// \file TestEnums.cxx
/// \brief Tests for enums
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/CardType.h"
#include "RORC/ParameterTypes/LoopbackMode.h"
#include "RORC/ParameterTypes/ReadoutMode.h"
#include "RORC/ParameterTypes/ResetLevel.h"

#define BOOST_TEST_MODULE RORC_TestEnums
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

using namespace AliceO2::Rorc;

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
  checkEnumConversion<CardType>({CardType::Crorc, CardType::Cru, CardType::Dummy, CardType::Unknown});
}

BOOST_AUTO_TEST_CASE(EnumLoopbackModeConversion)
{
  checkEnumConversion<LoopbackMode>({LoopbackMode::Diu, LoopbackMode::None, LoopbackMode::Rorc, LoopbackMode::Siu});
}

BOOST_AUTO_TEST_CASE(EnumResetLevelConversion)
{
  checkEnumConversion<ResetLevel>({ResetLevel::Nothing, ResetLevel::Rorc, ResetLevel::RorcDiu,
      ResetLevel::RorcDiuSiu});
}

BOOST_AUTO_TEST_CASE(EnumReadoutModeConversion)
{
  checkEnumConversion<ReadoutMode>({ReadoutMode::Continuous});
}
