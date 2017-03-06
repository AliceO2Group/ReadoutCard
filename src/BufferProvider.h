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

    void* getBufferStartAddress() const
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

    void* getReservedStartAddress() const
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

    void* getDmaStartAddress() const
    {
      return dmaStartAddress;
    }

    size_t getDmaSize() const
    {
      return dmaSize;
    }

  protected:
    void* bufferStartAddress;
    size_t bufferSize;
    void* reservedStartAddress;
    size_t reservedSize;
    size_t reservedOffset;
    void* dmaStartAddress;
    size_t dmaSize;
    size_t dmaOffset;
};

} // namespace Rorc
} // namespace AliceO2
