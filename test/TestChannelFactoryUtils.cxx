/// \file TestChannelFactoryUtils.cxx
/// \brief Test of the ChannelFactoryUtils class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/CardType.h"
#include "ReadoutCard/ChannelFactory.h"
#include "Factory/ChannelFactoryUtils.h"
#include <map>
#include <memory>

#define BOOST_TEST_MODULE RORC_TestChannelFactoryUtils
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

using namespace ::AliceO2::roc;

namespace {

struct TestInterface {
    TestInterface() {}
    virtual ~TestInterface() {}
};

struct DummyImpl : public TestInterface {};
struct CrorcImpl : public TestInterface {};
struct CruImpl : public TestInterface {};

// Helper function for calling the factory make function
std::unique_ptr<TestInterface> callMake()
{
  auto parameters = Parameters::makeParameters(ChannelFactory::getDummySerialNumber(), 0);
  return ChannelFactoryUtils::channelFactoryHelper<TestInterface>(parameters, ChannelFactory::getDummySerialNumber(), {
      {CardType::Dummy, []{ return std::make_unique<DummyImpl>(); }},
      {CardType::Crorc, []{ return std::make_unique<CrorcImpl>(); }},
      {CardType::Cru,   []{ return std::make_unique<CruImpl>(); }}
    });
}

// This tests if the FactoryHelper::make() function maps to the expected types.
// Unfortunately, we can't test the entire make function, since it depends on having actual PCI cards installed, so
// we test only part of its implementation.
BOOST_AUTO_TEST_CASE(ChannelFactoryHelperTest)
{
  BOOST_CHECK(nullptr != dynamic_cast<DummyImpl*>(callMake().get()));
}

} // Anonymous namespace
