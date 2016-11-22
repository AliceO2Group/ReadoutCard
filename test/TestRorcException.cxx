/// \file TestRorcException.cxx
/// \brief Test of the Exception class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ExceptionInternal.h"
#define BOOST_TEST_MODULE RORC_TestRorcException
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

using namespace AliceO2::Rorc;

namespace {

const std::string TEST_MESSAGE_1("test_message_1");
const std::string TEST_MESSAGE_2("test_message_2");
const std::string CAUSE_1("cause_1");
const std::string CAUSE_2("cause_2");

// Tests the what() function
BOOST_AUTO_TEST_CASE(TestRorcException)
{
  try {
    BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message(TEST_MESSAGE_1));
  }
  catch (const std::exception& e) {
    BOOST_CHECK(std::string(e.what()) == TEST_MESSAGE_1);
  }
}

// Tests if overwriting with a new message doesn't cause issues with using the old message
BOOST_AUTO_TEST_CASE(TestRorcException2)
{
  try {
    BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message(TEST_MESSAGE_1));
  }
  catch (Exception& e) {
    auto what1 = e.what();
    BOOST_CHECK(std::string(what1) == TEST_MESSAGE_1);

    // Overwrite old message
    e << errinfo_rorc_error_message(TEST_MESSAGE_2);

    // Will the old message now explode? Doesn't seem to be the case...
    BOOST_CHECK(std::string(what1) == TEST_MESSAGE_1);

    auto what2 = e.what();
    BOOST_CHECK(what1 != what2);
  }
}


// Tests the addPossibleCauses() function
BOOST_AUTO_TEST_CASE(TestAddCauses)
{
  try {
    auto e = Exception();
    e << errinfo_rorc_possible_causes({CAUSE_1});
    addPossibleCauses(e, {CAUSE_2});
    BOOST_THROW_EXCEPTION(e);
  }
  catch (const Exception& e) {
    auto causes = boost::get_error_info<errinfo_rorc_possible_causes>(e);
    BOOST_REQUIRE(causes != nullptr);
    BOOST_CHECK(causes->at(0) == CAUSE_1);
    BOOST_CHECK(causes->at(1) == CAUSE_2);
  }
}

// Tests the addPossibleCauses() function
BOOST_AUTO_TEST_CASE(TestAddCauses2)
{
  try {
    auto e = Exception();
    addPossibleCauses(e, {CAUSE_1});
    addPossibleCauses(e, {CAUSE_2});
    BOOST_THROW_EXCEPTION(e);
  }
  catch (const Exception& e) {
    auto causes = boost::get_error_info<errinfo_rorc_possible_causes>(e);
    BOOST_REQUIRE(causes != nullptr);
    BOOST_CHECK(causes->at(0) == CAUSE_1);
    BOOST_CHECK(causes->at(1) == CAUSE_2);
  }
}

// Tests the addPossibleCauses() function
BOOST_AUTO_TEST_CASE(TestAddCauses3)
{
  try {
    auto e = Exception();
    addPossibleCauses(e, {CAUSE_1});
    // This will overwrite the old info
    e << errinfo_rorc_possible_causes({CAUSE_2});
    BOOST_THROW_EXCEPTION(e);
  }
  catch (const Exception& e) {
    auto causes = boost::get_error_info<errinfo_rorc_possible_causes>(e);
    BOOST_REQUIRE(causes != nullptr);
    BOOST_CHECK(causes->size() == 1);
    BOOST_CHECK(causes->at(0) == CAUSE_2);
  }
}

} // Anonymous namespace
