///
/// \file FileSharedObject.h
/// \author Pascal Boeschoten
///

#pragma once

#include <boost/format.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {
namespace FileSharedObject {

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

// Tag arguments for FileSharedObject and LockFileSharedObject constructors
// They are needed to disambiguate between:
//  - The "find or construct" variant without any 'Args&&...' parameters
//  - The "find only" variant (which can't have 'Args&&...' parameters)
// Without these tags, those two constructors would look the same
/// Find or construct the shared object
const struct find_or_construct_tag {} find_or_construct;
/// Find the shared object, do not construct
const struct find_only_tag {} find_only;

/// Class for mapping a shared object stored in a file to a pointer
template <typename T>
class FileSharedObject
{
  public:
    /// This constructor will find or construct the shared object
    /// \param sharedFilePath   Path to the shared memory file for the object
    /// \param sharedFileSize   Size of the shared memory file
    /// \param sharedObjectName Name of the object within the shared memory file
    /// \param tag              Indicates that the shared object should be found or constructed
    /// \param args             Constructor arguments to be forwarded to the object
    template<typename ...Args>
    FileSharedObject(
        const bfs::path&      sharedFilePath,
        const size_t&         sharedFileSize,
        const std::string&    sharedObjectName,
        find_or_construct_tag tag,
        Args&&...             args
        )
        : sharedFile(bip::open_or_create, sharedFilePath.c_str(), sharedFileSize),
          sharedObjectPointer(sharedFile.find_or_construct<T>(sharedObjectName.c_str())(std::forward<Args>(args)...))
    {
    }

    /// This constructor will only find the shared object, not construct it
    /// \param sharedFilePath   Path to the shared memory file for the object
    /// \param sharedFileSize   Size of the shared memory file
    /// \param sharedObjectName Name of the object within the shared memory file
    /// \param tag              Indicates that the shared object should be found only
    /// \param args             Constructor arguments to be forwarded to the object
    template <typename ...Args>
    FileSharedObject(
        const bfs::path&   sharedFilePath,
        const size_t&      sharedFileSize,
        const std::string& sharedObjectName,
        find_only_tag      tag
        )
        : sharedFile(bip::open_or_create, sharedFilePath.c_str(), sharedFileSize),
          sharedObjectPointer(sharedFile.find<T>(sharedObjectName.c_str()).first)
    {
    }

    inline T* get()
    {
      return sharedObjectPointer;
    }

  private:
    bip::managed_mapped_file sharedFile;
    T* sharedObjectPointer;
};

/// Class for mapping a shared object stored in a file to a pointer, whose access is controlled by a file lock.
/// It wraps the FileSharedObject class.
/// Unfortunately, file locks are only guaranteed to work on a per-process basis.
/// Within a process, multiple threads may not be synchronized.
/// Therefore, the user of this object is still responsible for synchronizing access within the process.
/// For more information, refer to the 'File Locks' section of:
///   http://www.boost.org/doc/libs/1_55_0/doc/html/interprocess/synchronization_mechanisms.html
template <typename T>
class LockedFileSharedObject
{
  public:

    /// This constructor will find or construct the shared object
    /// \param lockPath         Path to lock file
    /// \param sharedFilePath   Path to the shared memory file for the object
    /// \param sharedFileSize   Size of the shared memory file
    /// \param sharedObjectName Name of the object within the shared memory file
    /// \param tag              Indicates that the shared object should be found or constructed
    /// \param args             Constructor arguments to be forwarded to the object
    template <typename ...Args>
    LockedFileSharedObject(
        const bfs::path&      lockPath,
        const bfs::path&      sharedFilePath,
        const size_t&         sharedFileSize,
        const std::string&    sharedObjectName,
        find_or_construct_tag tag,
        Args&&...             args
        )
        : fileLock(lockPath),
          fileSharedObject(sharedFilePath, sharedFileSize, sharedObjectName, tag,
              std::forward<Args>(args)...)
    {
    }

    /// This constructor will only find the shared object, not construct it
    /// \param lockPath         Path to lock file
    /// \param sharedFilePath   Path to the shared memory file for the object
    /// \param sharedFileSize   Size of the shared memory file
    /// \param sharedObjectName Name of the object within the shared memory file
    /// \param tag              Indicates that the shared object should be found or constructed
    /// \param args             Constructor arguments to be forwarded to the object
    template <typename ...Args>
    LockedFileSharedObject(
        const bfs::path&   lockPath,
        const bfs::path&   sharedFilePath,
        const size_t&      sharedFileSize,
        const std::string& sharedObjectName,
        find_only_tag      tag
        )
        : fileLock(lockPath),
          fileSharedObject(sharedFilePath, sharedFileSize, sharedObjectName, tag)
    {
    }

    inline T* get()
    {
      return fileSharedObject.get();
    }

  private:
    /// Helper class for file locks. Throws in constructor if it can't get a lock.
    class ThrowingFileLock
    {
      public:
        inline ThrowingFileLock(bfs::path fileLockPath)
            : lock(fileLockPath.c_str())
        {
          if (!lock.try_lock()) {
            ALICEO2_RORC_THROW_EXCEPTION(
                b::str(b::format("Failed to acquire file lock for '%s'") % fileLockPath.c_str()));
          }
        }

        inline ~ThrowingFileLock()
        {
          lock.unlock();
        }

        bip::file_lock lock;
    };

    ThrowingFileLock fileLock;
    FileSharedObject<T> fileSharedObject;
};

} // namespace FileSharedObject
} // namespace Rorc
} // namespace AliceO2

