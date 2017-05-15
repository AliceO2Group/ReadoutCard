/// \file TestChannelPaths.cxx
/// \brief Tests for ChannelPaths
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelPaths.h"

#define BOOST_TEST_MODULE RORC_TestChannelPaths
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include "CardDescriptor.h"

BOOST_AUTO_TEST_CASE(ChannelPathsTest)
{
  using namespace AliceO2::roc;
  ChannelPaths paths(PciAddress {0,0,0}, 0);
  BOOST_CHECK_NO_THROW(paths.fifo());
  BOOST_CHECK_NO_THROW(paths.lock());
  BOOST_CHECK_NO_THROW(paths.namedMutex());
}
