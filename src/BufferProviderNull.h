/// \file BufferProviderNull.h
/// \brief Definition of the BufferProviderNull class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "BufferProvider.h"

namespace AliceO2 {
namespace roc {

/// Buffer provider for no buffer at all
class BufferProviderNull : public BufferProvider
{
  public:
    BufferProviderNull()
    {
      initialize(nullptr, 0);
    }

    virtual ~BufferProviderNull()
    {
    }

  private:
};

} // namespace roc
} // namespace AliceO2
