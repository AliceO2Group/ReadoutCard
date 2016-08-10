/// \file TestChannelPaths.cxx
/// \brief Tests for ChannelPaths
///
/// \author Pascal Boeschoten

#include "ChannelPaths.h"

#define BOOST_TEST_MODULE RORC_TestChannelPaths
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>

BOOST_AUTO_TEST_CASE(ChannelPathsTest)
{
  using namespace AliceO2::Rorc;
  ChannelPaths paths(CardType::UNKNOWN, 0, 0);
  BOOST_CHECK_NO_THROW(paths.fifo());
  BOOST_CHECK_NO_THROW(paths.lock());
  BOOST_CHECK_NO_THROW(paths.namedMutex());
  BOOST_CHECK_NO_THROW(paths.pages());
  BOOST_CHECK_NO_THROW(paths.state());
}
