///
/// \file ReadyFifoWrapper.h
/// \author Pascal Boeschoten
///

#pragma once

#include <cstdint>

namespace AliceO2 {
namespace Rorc {

/// Wrapper for the CRORC "ready" FIFO
class ReadyFifoWrapper
{
  public:
    struct FifoEntry
    {
        int32_t length;
        int32_t status;

        inline void reset()
        {
          length = -1;
          status = -1;
        }
    };

    ReadyFifoWrapper(void* userAddress, void* deviceAddress, int length);
    void resetAll();
    void advanceWriteIndex();
    void advanceReadIndex();
    void advanceNextPage();
    FifoEntry& getWriteEntry();
    FifoEntry& getReadEntry();
    FifoEntry& getNextEntry();
    FifoEntry& getEntry(int i);
    int getLength() const;
    int getWriteIndex() const;
    int getReadIndex() const;
    int getNextPage() const;
    void* const getUserAddress() const;
    void* const getDeviceAddress() const;

  private:
    /// Index of page currently writing
    int writeIndex;

    /// Index of page currently reading
    int readIndex;

    /// Userspace address of the start of the FIFO
    FifoEntry* const userAddress;

    /// Device address of the start of the FIFO
    void* const deviceAddress;

    /// Amount of entries in the FIFO
    int length;
};

} // namespace Rorc
} // namespace AliceO2
