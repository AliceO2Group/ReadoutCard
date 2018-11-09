/// \file InterprocessLock.h
/// \brief Definitions for InterprocessLock class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_INTERPROCESSMUTEX_H_
#define ALICEO2_READOUTCARD_INTERPROCESSMUTEX_H_

#include <boost/exception/errinfo_errno.hpp>
#include <chrono>
#include "ExceptionInternal.h"
#include <sys/socket.h>
#include <sys/un.h>

#define LOCK_TIMEOUT 5 //5 second timeout in case we wait for the lock (e.g PDA)

namespace AliceO2 {
namespace roc {
namespace Interprocess {

class Lock
{
  public:

    Lock(const std::string &socketLockName, bool waitOnLock = false)
      :mSocketName(socketLockName)
    {

      if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        BOOST_THROW_EXCEPTION(LockException()
          << ErrorInfo::PossibleCauses({"Couldn't create socket fd"}));
      }

      memset (&server_address, 0, sizeof(server_address));
      server_address.sun_family = AF_UNIX;
      strcpy(server_address.sun_path, socketLockName.c_str());
      server_address.sun_path[0] = 0; //this makes the unix domain socket *abstract*

      if (waitOnLock) { //retry until timeout
        const auto start = std::chrono::steady_clock::now();
        auto timeExceeded = [&]() { return ((std::chrono::steady_clock::now() - start) > std::chrono::seconds(LOCK_TIMEOUT)); };

        while (bind(socket_fd,
              (const struct sockaddr *) &server_address,
              address_length) < 0 && !timeExceeded()) {}

        if (timeExceeded()) { //we timed out
          close(socket_fd);
          BOOST_THROW_EXCEPTION(LockException()
              << ErrorInfo::PossibleCauses({"Bind to socket timed out"}));
        }
      } else { //exit immediately after bind error
        if (bind(socket_fd,
              (const struct sockaddr *) &server_address,
              address_length) < 0) {
          close(socket_fd);
          BOOST_THROW_EXCEPTION(LockException()
              << ErrorInfo::PossibleCauses({"Couldn't bind to socket"}));
        }
      }
    }

    ~Lock()
    {
      close(socket_fd);
    }

  private:
    int socket_fd;
    struct sockaddr_un server_address;
    socklen_t address_length = sizeof(struct sockaddr_un);
    std::string mSocketName;
};

} // namespace Interprocess
} // namespace roc
} // namespace AliceO2

#endif /* ALICEO2_READOUTCARD_INTERPROCESSMUTEX_H_ */
