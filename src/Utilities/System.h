/// \file System.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <set>
#include <string>
#include <boost/filesystem/path.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Utilities {

/// Sets the given function as the SIGINT handler
void setSigIntHandler(void(*function)(int));

/// Checks if there's a SIGINT handler installed (not sure if it actually works correctly)
bool isSigIntHandlerSet();

std::string executeCommand(const std::string& command);

/// Like the "mkdir -p" command.
/// TODO Currently it actually calls that command.. not very portable, should refactor
void makeParentDirectories(const boost::filesystem::path& path);

/// Similar to the "touch" Linux command
void touchFile(const boost::filesystem::path& path);

/// Get the file system type of the given directory
std::string getFileSystemType(const boost::filesystem::path& path);

/// Check if the file system of the given directory is any of the types given in the set
/// \return pair with:
///   bool: true if it is any of types
///   std::string: the found type
std::pair<bool, std::string> isFileSystemTypeAnyOf(const boost::filesystem::path& path,
    const std::set<std::string>& types);

/// Throws if the file system type of the given file/directory is not one of the given valid types
void assertFileSystemType(std::string path, const std::set<std::string>& validTypes, std::string name);

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
