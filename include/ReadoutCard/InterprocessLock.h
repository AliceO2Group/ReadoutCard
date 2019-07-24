// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file InterprocessLock.h
/// \brief Definitions for InterprocessLock class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_INTERPROCESSMUTEX_H_
#define ALICEO2_READOUTCARD_INTERPROCESSMUTEX_H_

#include <boost/exception/errinfo_errno.hpp>
#include <chrono>
#include <sys/socket.h>
#include <sys/un.h>

#define LOCK_TIMEOUT 5            //5 second timeout in case we wait for the lock (e.g PDA)
#define UNIX_SOCK_NAME_LENGTH 104 //108 for most UNIXs, 104 for macOS

namespace AliceO2
{
namespace roc
{
namespace Interprocess
{

class Lock
{
 public:
  Lock(const std::string& socketLockName, bool waitOnLock = false)
    : mSocketName(socketLockName)
  {

    if ((mSocketFd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
      BOOST_THROW_EXCEPTION(std::runtime_error("Couldn't create abstract socket fd for InterprocessLock"));
    }

    memset(&mServerAddress, 0, sizeof(mServerAddress));
    mServerAddress.sun_family = AF_UNIX;
    // Care in case the filename is longer than the unix socket name length
    std::string safeSocketLockName = hashSocketLockName();
    strcpy(mServerAddress.sun_path, safeSocketLockName.c_str());
    mServerAddress.sun_path[0] = 0; //this makes the unix domain socket *abstract*

    if (waitOnLock) { //retry until timeout
      const auto start = std::chrono::steady_clock::now();
      auto timeExceeded = [&]() { return ((std::chrono::steady_clock::now() - start) > std::chrono::seconds(LOCK_TIMEOUT)); };

      while (bind(mSocketFd,
                  (const struct sockaddr*)&mServerAddress,
                  mAddressLength) < 0 &&
             !timeExceeded()) {
      }

      if (timeExceeded()) { //we timed out
        close(mSocketFd);
        BOOST_THROW_EXCEPTION(std::runtime_error("Bind to socket " + safeSocketLockName + " timed out"));
      }
    } else { //exit immediately after bind error
      if (bind(mSocketFd,
               (const struct sockaddr*)&mServerAddress,
               mAddressLength) < 0) {
        close(mSocketFd);
        BOOST_THROW_EXCEPTION(std::runtime_error("Couldn't bind to socket " + safeSocketLockName));
      }
    }
  }

  ~Lock()
  {
    close(mSocketFd);
  }

 private:
  std::string hashSocketLockName()
  {
    if (mSocketName.length() >= UNIX_SOCK_NAME_LENGTH) {
      std::string lockType = mSocketName.substr(0, 17);                               // isolate the class that created the lock
                                                                                      // Alice_O2_RoC_XYZ_*
      unsigned long lockNameHash = hashDjb2(mSocketName.c_str(), mSocketName.size()); // hash the mutable part
      std::string safeLockName = lockType + std::to_string(lockNameHash) + "_lock";   // return conformant name
      return safeLockName;
    } else {
      return mSocketName;
    }
  }

  unsigned long hashDjb2(const char* str, size_t length)
  {
    unsigned long hash = 5381;
    for (size_t i = 0; i < length; ++i)
      hash = 33 * hash + (unsigned char)str[i];
    return hash;
  }

  int mSocketFd;
  struct sockaddr_un mServerAddress;
  socklen_t mAddressLength = sizeof(struct sockaddr_un);
  std::string mSocketName;
};

} // namespace Interprocess
} // namespace roc
} // namespace AliceO2

#endif /* ALICEO2_READOUTCARD_INTERPROCESSMUTEX_H_ */
