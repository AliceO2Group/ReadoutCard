/// \file Thread.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <boost/throw_exception.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include "ExceptionInternal.h"
#include "InfoLogger/InfoLogger.hxx"


namespace AliceO2 {
namespace Rorc {
namespace Utilities {

class Thread
{
  public:

    void start(std::function<void(std::atomic<bool>* stopFlag)> function)
    {
      join();
      mStopFlag = false;
      mThread = std::thread(function, &mStopFlag);
    }

    void stop()
    {
      mStopFlag = true;
    }

    void join()
    {
      stop();
      if (mThread.joinable()) {
        mThread.join();
      }
    }

    ~Thread()
    {
      try {
        join();
      }
      catch (const std::system_error& e) {
        std::cout << "Failed to join thread: " << boost::diagnostic_information(e) << '\n';
      }
      catch (const std::exception& e) {
        std::cout << "Unexpected exception while joining thread: " << boost::diagnostic_information(e) << '\n';
      }
    }

  private:
    /// Thread object
    std::thread mThread;

    /// Flag to stop the thread
    std::atomic<bool> mStopFlag;
};

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
