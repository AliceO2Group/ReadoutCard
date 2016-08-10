/// \file TestEnums.cxx
/// \brief Tests for enums
///
/// \author Pascal Boeschoten

#include <RORC/RORC.h>
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
  checkEnumConversion<CardType>({CardType::CRORC, CardType::CRU, CardType::DUMMY, CardType::UNKNOWN});
}

BOOST_AUTO_TEST_CASE(EnumLoopbackModeConversion)
{
  using namespace AliceO2::Rorc;
  checkEnumConversion<LoopbackMode>({LoopbackMode::DIU, LoopbackMode::NONE, LoopbackMode::RORC, LoopbackMode::SIU});
}

BOOST_AUTO_TEST_CASE(EnumResetLevelConversion)
{
  using namespace AliceO2::Rorc;
  checkEnumConversion<ResetLevel>({ResetLevel::NOTHING, ResetLevel::RORC, ResetLevel::RORC_DIU,
      ResetLevel::RORC_DIU_SIU});
}

