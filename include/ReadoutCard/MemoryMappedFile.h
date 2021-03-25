// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file MemoryMappedFile.h
/// \brief Definition of the MemoryMappedFile class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_MEMORYMAPPEDFILE_H_
#define O2_READOUTCARD_INCLUDE_MEMORYMAPPEDFILE_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>
#include <memory>
#include "ReadoutCard/InterprocessLock.h"

namespace o2
{
namespace roc
{

struct MemoryMappedFileInternal;

/// Handles the creation and cleanup of a memory mapping of a file
class MemoryMappedFile
{
 public:
  MemoryMappedFile();

  MemoryMappedFile(const std::string& fileName, size_t fileSize, bool deleteFileOnDestruction = false, bool lockMap = true);

  ~MemoryMappedFile();

  void* getAddress() const;

  size_t getSize() const;

  std::string getFileName() const;

 private:
  bool map(const std::string& fileName, size_t fileSize);

  std::unique_ptr<MemoryMappedFileInternal> mInternal;

  std::unique_ptr<Interprocess::Lock> mInterprocessLock;

  // Flag that shows if the map was succesful
  bool mMapAcquired = false;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_MEMORYMAPPEDFILE_H_
