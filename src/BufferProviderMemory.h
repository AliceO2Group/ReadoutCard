/// \file BufferProvider.h
/// \brief Definition of the BufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "BufferProvider.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {

/// Buffer provider for a buffer that is already available in memory
class BufferProviderMemory : public BufferProvider
{
  public:
    BufferProviderMemory(const buffer_parameters::Memory& parameters)
    {
      initialize(parameters.address, parameters.size);
    }

    virtual ~BufferProviderMemory()
    {
    }
};

} // namespace roc
} // namespace AliceO2
