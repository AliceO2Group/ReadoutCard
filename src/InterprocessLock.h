/// \file InterprocessLock.h
/// \brief Definitions for InterprocessMutex class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_RORC_INTERPROCESSMUTEX_H_
#define ALICEO2_RORC_INTERPROCESSMUTEX_H_

#include <mutex>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/scoped_ptr.hpp>
#include "Util.h"

namespace AliceO2 {
namespace Rorc {
namespace Interprocess {

/// This lock uses two mutexes internally to help detect inconsistent state.
/// When the process is terminated unexpectedly, the The boost::file_lock is unlocked automatically, while the
/// boost::interprocess_mutex IS NOT unlocked automatically. This gives us some detection ability.
class Lock
{
  public:
    using FileMutex = boost::interprocess::file_lock;
    using NamedMutex = boost::interprocess::named_mutex;
    using FileLock = std::unique_lock<FileMutex>;
    using NamedLock = std::unique_lock<NamedMutex>;

    Lock(const boost::filesystem::path& lockFilePath, const std::string& namedMutexName)
    {
      Util::touchFile(lockFilePath);

      // Construct mutexes
      try {
        Util::resetSmartPtr(mFileMutex, lockFilePath.c_str());
      }
      catch (const boost::interprocess::interprocess_exception& e) {
        BOOST_THROW_EXCEPTION(LockException()
            << errinfo_rorc_error_message(e.what())
            << errinfo_rorc_shared_lock_file(lockFilePath.string())
            << errinfo_rorc_named_mutex_name(namedMutexName)
            << errinfo_rorc_possible_causes({"Invalid lock file path"}));
      }
      try {
        Util::resetSmartPtr(mNamedMutex, boost::interprocess::open_or_create, namedMutexName.c_str());
      }
      catch (const boost::interprocess::interprocess_exception& e) {
        BOOST_THROW_EXCEPTION(LockException()
            << errinfo_rorc_error_message(e.what())
            << errinfo_rorc_shared_lock_file(lockFilePath.string())
            << errinfo_rorc_named_mutex_name(namedMutexName)
            << errinfo_rorc_possible_causes({"Invalid mutex name (note: it should be a name, not a path)"}));
      }

      // Initialize locks
      mFileLock = FileLock(*mFileMutex.get(), std::defer_lock);
      mNamedLock = NamedLock(*mNamedMutex.get(), std::defer_lock);

      // Attempt to lock mutexes
      int result = std::try_lock(mFileLock, mNamedLock);
      if (result == 0) {
        // First lock failed
        BOOST_THROW_EXCEPTION(FileLockException()
            << errinfo_rorc_error_message("Failed to acquire file lock")
            << errinfo_rorc_shared_lock_file(lockFilePath.string())
            << errinfo_rorc_named_mutex_name(namedMutexName));
      } else if (result == 1) {
        // Second lock failed
        BOOST_THROW_EXCEPTION(NamedMutexLockException()
            << errinfo_rorc_error_message("Failed to acquire named mutex only; file lock was successfully acquired")
            << errinfo_rorc_shared_lock_file(lockFilePath.string())
            << errinfo_rorc_named_mutex_name(namedMutexName)
            << errinfo_rorc_possible_causes({"Named mutex is owned by other thread in current process",
                "Previous Interprocess::Lock on same objects was not cleanly destroyed"}));
      }
    }

  private:
    boost::scoped_ptr<FileMutex> mFileMutex;
    boost::scoped_ptr<NamedMutex> mNamedMutex;
    FileLock mFileLock;
    NamedLock mNamedLock;
};

} // namespace Interprocess
} // namespace Rorc
} // namespace AliceO2

#endif /* ALICEO2_RORC_INTERPROCESSMUTEX_H_ */
