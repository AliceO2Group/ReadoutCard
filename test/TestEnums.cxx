/// \file TestEnums.cxx
/// \brief Tests for enums
///
/// \author Pascal Boeschoten

#include "RORC/RORC.h"
#include "Utilities/Options.h"

#define BOOST_TEST_MODULE RORC_TestEnums
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

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
  using namespace AliceO2::Rorc;
  checkEnumConversion<CardType>({CardType::Crorc, CardType::Cru, CardType::Dummy, CardType::Unknown});
}

BOOST_AUTO_TEST_CASE(EnumLoopbackModeConversion)
{
  using namespace AliceO2::Rorc;
  checkEnumConversion<LoopbackMode>({LoopbackMode::Diu, LoopbackMode::None, LoopbackMode::Rorc, LoopbackMode::Siu});
}

BOOST_AUTO_TEST_CASE(EnumResetLevelConversion)
{
  using namespace AliceO2::Rorc;
  checkEnumConversion<ResetLevel>({ResetLevel::Nothing, ResetLevel::Rorc, ResetLevel::RorcDiu,
      ResetLevel::RorcDiuSiu});
}

