/// \file BufferProvider.h
/// \brief Definition of the BufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

namespace AliceO2 {
namespace Rorc {

/// Base class for objects that provide addresses and offsets for buffers (typically DMA buffers)
class BufferProvider
{
  public:
    virtual ~BufferProvider()
    {
    }

    uintptr_t getBufferStartAddress() const
    {
      return reservedStartAddress;
    }

    size_t getBufferSize() const
    {
      return bufferSize;
    }

    size_t getReservedOffset() const
    {
      return reservedOffset;
    }

    uintptr_t getReservedStartAddress() const
    {
      return reservedStartAddress;
    }

    size_t getReservedSize() const
    {
      return reservedSize;
    }

    size_t getDmaOffset() const
    {
      return dmaOffset;
    }

    uintptr_t getDmaStartAddress() const
    {
      return dmaStartAddress;
    }

    size_t getDmaSize() const
    {
      return dmaSize;
    }

  protected:
    uintptr_t bufferStartAddress;
    size_t bufferSize;
    uintptr_t reservedStartAddress;
    size_t reservedSize;
    size_t reservedOffset;
    uintptr_t dmaStartAddress;
    size_t dmaSize;
    size_t dmaOffset;
};

} // namespace Rorc
} // namespace AliceO2
