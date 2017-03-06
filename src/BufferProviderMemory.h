/// \file BufferProvider.h
/// \brief Definition of the BufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "BufferProvider.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace Rorc {

/// Buffer provider for a buffer that is already available in memory
class BufferProviderMemory : public BufferProvider
{
  public:
    BufferProviderMemory(const BufferParameters::Memory& parameters)
    {
      bufferStartAddress = reinterpret_cast<uintptr_t>(parameters.bufferStart);
      bufferSize = parameters.bufferSize;

      reservedStartAddress = reinterpret_cast<uintptr_t>(parameters.reservedStart);
      reservedSize = parameters.reservedSize;
      if (reservedStartAddress < bufferStartAddress) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("BufferParameters::Memory reservedStart < bufferStart"));
      }
      reservedOffset = reservedStartAddress - bufferStartAddress;
      if ((reservedStartAddress + reservedSize) > (bufferStartAddress + bufferSize)) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Reserved region out of range"));
      }

      dmaStartAddress = reinterpret_cast<uintptr_t>(parameters.dmaStart);
      dmaSize = parameters.dmaSize;
      if (dmaStartAddress < bufferStartAddress) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("BufferParameters::Memory dmaStart < bufferStart"));
      }
      dmaOffset = dmaStartAddress - bufferStartAddress;
      if ((dmaStartAddress + reservedSize) > (bufferStartAddress + bufferSize)) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("DMA region out of range"));
      }
    }

    virtual ~BufferProviderMemory()
    {
    }
};

} // namespace Rorc
} // namespace AliceO2
