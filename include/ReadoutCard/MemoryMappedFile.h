/// \file MemoryMappedFile.h
/// \brief Definition of the MemoryMappedFile class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_MEMORYMAPPEDFILE_H_
#define ALICEO2_INCLUDE_READOUTCARD_MEMORYMAPPEDFILE_H_

#include <string>
#include <memory>

namespace AliceO2 {
namespace roc {

struct MemoryMappedFileInternal;

/// Handles the creation and cleanup of a memory mapping of a file
class MemoryMappedFile
{
  public:
    MemoryMappedFile();

    MemoryMappedFile(const std::string& fileName, size_t fileSize, bool deleteFileOnDestruction = false);

    ~MemoryMappedFile();

    void* getAddress() const;

    size_t getSize() const;

    std::string getFileName() const;

  private:
    void map(const std::string& fileName, size_t fileSize);

    std::unique_ptr<MemoryMappedFileInternal> mInternal;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_MEMORYMAPPEDFILE_H_
