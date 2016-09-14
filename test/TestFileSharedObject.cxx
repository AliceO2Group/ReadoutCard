///
/// \file TestFileSharedObject.cxx
/// \author Pascal Boeschoten
///

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

using SharedObject = FileSharedObject::LockedFileSharedObject<TestObject>;

const std::string objectName("ObjectName");
const std::string mutexName("AliceO2_FileSharedObject_TestMutex");
const bfs::path filePath("/tmp/AliceO2_FileSharedObject_Test");
const bfs::path lockPath("/tmp/AliceO2_FileSharedObject_Test.lock");
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
  bfs::remove(lockPath);
  bfs::remove(filePath);
  bip::named_mutex::remove(mutexName.c_str());
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

BOOST_AUTO_TEST_CASE(LockedFileSharedObjectFindOnlyTest)
{
  cleanupFiles();

  // Should fail with find_only: the file doesn't exist so there's nothing to find
  BOOST_CHECK_EXCEPTION(FileSharedObject::LockedFileSharedObject<TestObject>(lockPath, filePath, fileSize, objectName,
      FileSharedObject::find_only), SharedObjectNotFoundException, checkException);

  cleanupFiles();

  // Should succeed with find_or_construct: will automatically create the files and construct the object
  BOOST_CHECK_NO_THROW
  ({
    FileSharedObject::LockedFileSharedObject<TestObject> fso(lockPath, filePath, fileSize, objectName,
        FileSharedObject::find_or_construct, referenceString, referenceInteger);

    // Insert reference values
    putReferenceValues(fso);
  });

  // Check if the reference values are correct
  BOOST_CHECK_NO_THROW
  ({
    FileSharedObject::LockedFileSharedObject<TestObject> fso(lockPath, filePath, fileSize, objectName,
        FileSharedObject::find_or_construct, referenceString, referenceInteger);

    checkReferenceValues(fso);
  });

  cleanupFiles();
}

// Just hammer it and see if it breaks
BOOST_AUTO_TEST_CASE(LockedFileSharedObjectRepeatedTest)
{
  cleanupFiles();
  for (int i = 0; i < 50; ++i) {
    BOOST_CHECK_NO_THROW(FileSharedObject::LockedFileSharedObject<TestObject> fso(lockPath, filePath, fileSize, objectName,
        FileSharedObject::find_or_construct, referenceString, referenceInteger));
  }
}

// Test the intraprocess file locking.
// Since this relies on a file lock it is somewhat OS dependent.
// On Linux, it will not prevent multiple threads from acquiring the lock, only multiple processes.
BOOST_AUTO_TEST_CASE(LockFileSharedObjectIntraprocessTest)
{
  cleanupFiles();

  std::condition_variable conditionVariable;
  std::atomic<bool> childAcquired(false);

  auto future = std::async(std::launch::async, [&](){
    // Child
    try {
      FileSharedObject::LockedFileSharedObject<TestObject> fso(lockPath, filePath, fileSize, objectName,
          FileSharedObject::find_or_construct, referenceString, referenceInteger);
      childAcquired = true;
      conditionVariable.notify_all();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    catch (const LockException& e) {
      BOOST_FAIL("Child failed to acquire FileSharedObject");
    }
  });

  // Parent
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  auto status = conditionVariable.wait_for(lock, std::chrono::milliseconds(100), [&](){ return bool(childAcquired); });
  if (!status) {
    BOOST_FAIL("Timed out or child failed to acquire lock");
  }

  // Since the file lock doesn't synchronize between threads, only processes, this should not fail.
  BOOST_CHECK_NO_THROW(FileSharedObject::LockedFileSharedObject<TestObject> fso(lockPath, filePath, fileSize, objectName,
      FileSharedObject::find_or_construct, referenceString, referenceInteger));
  future.get();
}

// Check if the ThrowingLockGuard locks properly between threads, using boost::interprocess::named_mutex
BOOST_AUTO_TEST_CASE(BoostIPCMutex)
{
  using Mutex = boost::interprocess::named_mutex;
  using Guard = FileSharedObject::ThrowingLockGuard<Mutex>;
  Mutex mutex(bip::open_or_create, mutexName.c_str());

  std::condition_variable conditionVariable;
  std::atomic<bool> childAcquired(false);

  auto future = std::async(std::launch::async, [&](){
    // Child
    Guard guard(&mutex);
    childAcquired = true;
    conditionVariable.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return;
  });

  // Parent
  {
    std::mutex conditionMutex;
    std::unique_lock<std::mutex> lock(conditionMutex);
    auto status = conditionVariable.wait_for(lock, std::chrono::milliseconds(100), [&](){ return bool(childAcquired); });
    if (!status) {
      BOOST_FAIL("Timed out or child failed to acquire lock");
    }
  }

  BOOST_CHECK_THROW(Guard guard(&mutex), LockException);
  future.get();
}


// Test the interprocess file locking. Probably not 100% reliable test, since we rely on sleeps to "synchronize"
// The idea is:
// - The parent waits a bit
// - The child locks immediately and holds the lock for a while
// - The parent tries to acquire while the child has it -> it should fail
BOOST_AUTO_TEST_CASE(LockFileSharedObjectInterprocessTest)
{
  cleanupFiles();
  pid_t pid = fork();

  // Reverse case: parent should FAIL to acquire the lock
  if (pid > 0) {
    // Parent
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    BOOST_CHECK_THROW(FileSharedObject::LockedFileSharedObject<TestObject> fso(lockPath, filePath, fileSize, objectName,
        FileSharedObject::find_or_construct, referenceString, referenceInteger), FileLockException);
  }
  else if (pid == 0) {
    // Child
    {
      FileSharedObject::LockedFileSharedObject<TestObject> fso(lockPath, filePath, fileSize, objectName,
          FileSharedObject::find_or_construct, referenceString, referenceInteger);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    exit(0);
  }
  else {
    BOOST_FAIL("Failed to fork");
  }
}

} // Anonymous namespace
