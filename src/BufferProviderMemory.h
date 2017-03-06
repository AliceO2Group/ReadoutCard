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
      bufferStartAddress = parameters.bufferStart;
      bufferSize = parameters.bufferSize;
      reservedOffset = Utilities::pointerDiff(parameters.reservedStart, parameters.bufferStart);
      reservedStartAddress = parameters.reservedStart;
      reservedSize = parameters.reservedSize;
      dmaOffset = Utilities::pointerDiff(parameters.dmaStart, parameters.bufferStart);
      dmaStartAddress = parameters.dmaStart;
      dmaSize = parameters.dmaSize;
    }

    virtual ~BufferProviderMemory()
    {
    }
};

} // namespace Rorc
} // namespace AliceO2
