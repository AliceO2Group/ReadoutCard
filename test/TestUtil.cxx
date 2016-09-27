/// \file TestUtil.cxx
/// \brief Test of the Util class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#define BOOST_TEST_MODULE RORC_TestChannelFactoryUtils
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>
#include "Util.h"

using namespace ::AliceO2::Rorc;

namespace {

BOOST_AUTO_TEST_CASE(ChannelFactoryHelperTest)
{
  std::vector<std::string> strings { "hello", "1.23", "42" };

  std::string x;
  double y;
  int z;
  BOOST_CHECK_NO_THROW(Util::convertAssign(strings, x, y, z));
  BOOST_CHECK(x == "hello");
  BOOST_CHECK(y == 1.23);
  BOOST_CHECK(z == 42);

  int tooMany;
  BOOST_CHECK_THROW(Util::convertAssign(strings, x, y, z, tooMany), UtilException);


}

} // Anonymous namespace
