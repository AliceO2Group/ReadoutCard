/// \file TestFileSharedObject.cxx
/// \brief Test of the FileSharedObject class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "FileSharedObject.h"

#define BOOST_TEST_MODULE RORC_TestFileSharedObject
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <assert.h>
#include <string>
#include <thread>
#include <future>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/exception/all.hpp>

#include "RORC/Exception.h"

namespace {

using namespace ::AliceO2::Rorc;
namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

struct TestObject
{
    TestObject(const std::string& s, int i) : string(s), integer(i) {}
    std::string string;
    int integer;
};

const std::string objectName("ObjectName");
const bfs::path filePath("/tmp/AliceO2_FileSharedObject_Test");
const size_t fileSize(4 *1024);

const std::string referenceString("HelloTest!");
const int referenceInteger(0x1234);

bool checkException(const SharedObjectNotFoundException& e)
{
  auto errorFile = boost::get_error_info<errinfo_rorc_filename>(e);
  auto errorObjectName = boost::get_error_info<errinfo_rorc_shared_object_name>(e);

  if (errorFile && errorObjectName) {
    return (*errorFile == filePath) && (*errorObjectName == objectName);
  } else {
    return false;
  }
}

void cleanupFiles()
{
  bfs::remove(filePath);
}

template <typename FSO>
void putReferenceValues(FSO& fso)
{
  fso.get()->string = referenceString;
  fso.get()->integer = referenceInteger;
}

template <typename FSO>
void checkReferenceValues(FSO& fso)
{
  BOOST_CHECK(fso.get()->string == referenceString);
  BOOST_CHECK(fso.get()->integer == referenceInteger);
}

BOOST_AUTO_TEST_CASE(FileSharedObjectFindOnlyTest)
{
  cleanupFiles();

  // Should fail with find_only: the file doesn't exist so there's nothing to find
  BOOST_CHECK_EXCEPTION(FileSharedObject::FileSharedObject<TestObject>(filePath, fileSize, objectName,
      FileSharedObject::find_only), SharedObjectNotFoundException, checkException);

  cleanupFiles();

  // Should succeed with find_or_construct: will automatically create the file and construct the object
  BOOST_CHECK_NO_THROW
  ({
    FileSharedObject::FileSharedObject<TestObject> fso(filePath, fileSize, objectName,
        FileSharedObject::find_or_construct, referenceString, referenceInteger);

    // Insert reference values
    putReferenceValues(fso);
  });

  // Check if the reference values are correct
  BOOST_CHECK_NO_THROW
  ({
    FileSharedObject::FileSharedObject<TestObject> fso(filePath, fileSize, objectName,
        FileSharedObject::find_or_construct, referenceString, referenceInteger);

    checkReferenceValues(fso);
  });

  cleanupFiles();
}

} // Anonymous namespace
