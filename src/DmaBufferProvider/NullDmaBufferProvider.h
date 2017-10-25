/// \file NullDmaBufferProvider.h
/// \brief Definition of the NullDmaBufferProvider.h class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "DmaBufferProvider/DmaBufferProviderInterface.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include "ExceptionInternal.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaDmaBuffer.h"

namespace AliceO2 {
namespace roc {

/// Base class for objects that provides addresses and offsets for buffers (typically DMA buffers)
class NullDmaBufferProvider : public DmaBufferProviderInterface
{
  public:
    virtual ~NullDmaBufferProvider() = default;

    /// Get starting userspace address of the DMA buffer
    virtual uintptr_t getAddress() const
    {
      return 0;
    }

    /// Get total size of the DMA buffer
    virtual size_t getSize() const
    {
      return 0;
    }

    /// Amount of entries in the scatter-gather list
    virtual size_t getScatterGatherListSize() const
    {
      return 0;
    }

    /// Get size of an entry of the scatter-gather list
    virtual size_t getScatterGatherEntrySize(int index) const
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("No scatter-gather list provided"));
    }

    /// Get userspace address of an entry of the scatter-gather list
    virtual uintptr_t getScatterGatherEntryAddress(int index) const
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("No scatter-gather list provided"));
    }

    /// Function for getting the bus address that corresponds to the user address + given offset
    virtual uintptr_t getBusOffsetAddress(size_t offset) const
    {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("No bus addresses provided"));
    }
};

} // namespace roc
} // namespace AliceO2
