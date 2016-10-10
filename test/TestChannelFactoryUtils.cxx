/// \file TestChannelFactoryUtils.cxx
/// \brief Test of the ChannelFactoryUtils class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/CardType.h"
#include "Factory/ChannelFactoryUtils.h"
#include <map>
#include <memory>

#define BOOST_TEST_MODULE RORC_TestChannelFactoryUtils
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

using namespace ::AliceO2::Rorc;
using namespace ::AliceO2::Rorc::CardTypeTag;

namespace {

struct TestInterface {
    TestInterface() {}
    virtual ~TestInterface() {}
};

struct DummyImpl : public TestInterface {};
struct CrorcImpl : public TestInterface {};
struct CruImpl : public TestInterface {};

// Helper function for deducing template arguments
template<class ...Args>
std::shared_ptr<TestInterface> _callMake(CardType::type cardType, Args&&... args)
{
  return FactoryHelper::_make_impl::Make<std::shared_ptr<TestInterface>, 0, Args...>::make(cardType,
      std::forward<Args>(args)...);
}

// Helper function for calling the factory make function
std::shared_ptr<TestInterface> callMake(CardType::type cardType)
{
  return _callMake(cardType,
      DummyTag, []{ return std::make_shared<DummyImpl>(); },
      CrorcTag, []{ return std::make_shared<CrorcImpl>(); },
      CruTag,   []{ return std::make_shared<CruImpl>(); });
}

// This tests if the FactoryHelper::make() function maps to the expected types.
// Unfortunately, we can't test the entire make function, since it depends on having actual PCI cards installed, so
// we test only part of its implementation.
BOOST_AUTO_TEST_CASE(ChannelFactoryHelperTest)
{
  BOOST_CHECK(nullptr != dynamic_cast<DummyImpl*>(callMake(CardType::Dummy).get()));
  BOOST_CHECK(nullptr != dynamic_cast<CrorcImpl*>(callMake(CardType::Crorc).get()));
  BOOST_CHECK(nullptr != dynamic_cast<CruImpl*>(callMake(CardType::Cru).get()));
}

} // Anonymous namespace
