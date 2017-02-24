/// \file MemoryMappedFile.h
/// \brief Definition of the MemoryMappedFile class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <string>
#include <memory>

namespace AliceO2 {
namespace Rorc {

struct MemoryMappedFileInternal;

/// Handles the creation and cleanup of a memory mapping of a file
class MemoryMappedFile
{
  public:
    MemoryMappedFile();

    MemoryMappedFile(const std::string& fileName, size_t fileSize);

    ~MemoryMappedFile();

    void* getAddress() const;

    size_t getSize() const;

  private:
    void map(const std::string& fileName, size_t fileSize);

    std::unique_ptr<MemoryMappedFileInternal> mInternal;
};

} // namespace Rorc
} // namespace AliceO2
