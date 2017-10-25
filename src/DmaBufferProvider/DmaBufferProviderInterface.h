/// \file DmaBufferProviderInterface.h
/// \brief Definition of the BufferProviderInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace AliceO2 {
namespace roc {

/// Base class for objects that provides addresses and offsets for buffers (typically DMA buffers)
class DmaBufferProviderInterface
{
  public:
    virtual ~DmaBufferProviderInterface()
    {
    }

    /// Get starting userspace address of the DMA buffer
    virtual uintptr_t getAddress() const = 0;

    /// Get total size of the DMA buffer
    virtual size_t getSize() const = 0;

    /// Amount of entries in the scatter-gather list
    virtual size_t getScatterGatherListSize() const = 0;

    /// Get size of an entry of the scatter-gather list
    virtual size_t getScatterGatherEntrySize(int index) const = 0;

    /// Get userspace address of an entry of the scatter-gather list
    virtual uintptr_t getScatterGatherEntryAddress(int index) const = 0;

    /// Function for getting the bus address that corresponds to the user address + given offset
    virtual uintptr_t getBusOffsetAddress(size_t offset) const = 0;
};

} // namespace roc
} // namespace AliceO2
