/// \file BufferProvider.h
/// \brief Definition of the BufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

namespace AliceO2 {
namespace roc {

/// Base class for objects that provides addresses and offsets for buffers (typically DMA buffers)
class BufferProvider
{
  public:
    virtual ~BufferProvider()
    {
    }

    void* getAddress() const
    {
      return mAddress;
    }

    size_t getSize() const
    {
      return mSize;
    }

  protected:

    void initialize(uintptr_t address, size_t size)
    {
      initialize(reinterpret_cast<void*>(address), size);

    }

    void initialize(void* address, size_t size)
    {
      mAddress = address;
      mSize = size;
    }

  private:
    void* mAddress = 0;
    size_t mSize = 0;
};

} // namespace roc
} // namespace AliceO2
