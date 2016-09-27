/// \file TestInterprocessLock.cxx
/// \brief Test of the InterprocessLock class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "FileSharedObject.h"

#define BOOST_TEST_MODULE RORC_TestFileSharedObject
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <assert.h>
#include <string>
#include <thread>
#include <iostream>
#include <future>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include "InterprocessLock.h"

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

const std::string namedMutexName("AliceO2_InterprocessMutex_Test");
const bfs::path lockFilePath("/tmp/AliceO2_InterprocessMutex_Test.lock");

void cleanupFiles()
{
  bip::named_mutex::remove(namedMutexName.c_str());
  bfs::remove(lockFilePath);
}

#define CONSTRUCT_LOCK() Interprocess::Lock _test_lock {lockFilePath, namedMutexName}

void doLock()
{
  CONSTRUCT_LOCK();
}

// Test the intraprocess locking
BOOST_AUTO_TEST_CASE(InterprocessMutexTestIntraprocess)
{
  cleanupFiles();

  std::condition_variable conditionVariable;
  std::atomic<bool> childAcquired(false);

  // Launch "child" thread that acquires the lock
  auto future = std::async(std::launch::async, [&](){
    try {
      CONSTRUCT_LOCK();
      childAcquired = true;
      conditionVariable.notify_all();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    catch (const LockException& e) {
      BOOST_FAIL("Child failed to acquire FileSharedObject");
    }
  });

  // Parent tries to acquire the lock
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
  auto status = conditionVariable.wait_for(lock, std::chrono::milliseconds(100), [&]{return childAcquired.load();});
  if (!status) {
    BOOST_FAIL("Timed out or child failed to acquire lock");
  }
  BOOST_CHECK_THROW(doLock(), NamedMutexLockException);

  // Wait on child thread
  future.get();
}


// Test the interprocess locking. Probably not 100% reliable test, since we rely on sleeps to "synchronize"
// The procedure is:
// - The parent waits a bit
// - The child locks immediately and holds the lock for a while
// - The parent tries to acquire while the child has it -> it should fail
BOOST_AUTO_TEST_CASE(InterprocessMutexTestInterprocess)
{
  cleanupFiles();

  pid_t pid = fork();

  if (pid == 0) {
    // Child
    { // We add this scope so the lock destroys cleanly before exit() is called
      CONSTRUCT_LOCK();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    cleanupFiles();
    exit(0);
  }
  else if (pid > 0) {
    // Parent
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    BOOST_CHECK_THROW(doLock(), FileLockException);
  }
  else {
    BOOST_FAIL("Failed to fork");
  }
}

} // Anonymous namespace
