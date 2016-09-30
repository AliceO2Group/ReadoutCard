/// \file FileSharedObject.h
/// \brief Definition of the FileSharedObject class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <boost/format.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "RORC/Exception.h"
#include "Util.h"

namespace AliceO2 {
namespace Rorc {
namespace FileSharedObject {

// Tag arguments for FileSharedObject and LockFileSharedObject constructors
// They are needed to disambiguate between:
//  - The "find or construct" variant without any 'Args&&...' parameters
//  - The "find only" variant (which can't have 'Args&&...' parameters)
// Without these tags, those two constructors would look the same
/// Find or construct the shared object
constexpr struct find_or_construct_tag {} find_or_construct = {};

/// Find the shared object, do not construct
constexpr struct find_only_tag {} find_only = {};

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
        const boost::filesystem::path& sharedFilePath,
        const size_t&                  sharedFileSize,
        const std::string&             sharedObjectName,
        find_or_construct_tag          ,
        Args&&...                      args)
        : mSharedFile(boost::interprocess::open_or_create, sharedFilePath.c_str(), sharedFileSize),
          mSharedObjectPointer(mSharedFile.find_or_construct<T>(sharedObjectName.c_str())(std::forward<Args>(args)...))
    {
    }

    /// This constructor will only find the shared object, not construct it
    /// \param sharedFilePath   Path to the shared memory file for the object
    /// \param sharedFileSize   Size of the shared memory file
    /// \param sharedObjectName Name of the object within the shared memory file
    /// \param tag              Indicates that the shared object should be found only
    FileSharedObject(
        const boost::filesystem::path& sharedFilePath,
        const size_t&                  sharedFileSize,
        const std::string&             sharedObjectName,
        find_only_tag)
        : mSharedFile(boost::interprocess::open_or_create, sharedFilePath.c_str(), sharedFileSize),
          mSharedObjectPointer(mSharedFile.find<T>(sharedObjectName.c_str()).first)
    {
      if (mSharedObjectPointer == nullptr) {
        BOOST_THROW_EXCEPTION(SharedObjectNotFoundException()
            << errinfo_rorc_filename(sharedFilePath.string())
            << errinfo_rorc_shared_object_name(sharedObjectName));
      }
    }

    T* get()
    {
      return mSharedObjectPointer;
    }

  private:
    boost::interprocess::managed_mapped_file mSharedFile;

    /// Because the managed_mapped_file's find_or_construct() is not trivial, we "cache" the result in this pointer
    T* mSharedObjectPointer;
};

} // namespace FileSharedObject
} // namespace Rorc
} // namespace AliceO2

