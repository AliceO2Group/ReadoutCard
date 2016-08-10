///
/// \file PageHandle.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include "ChannelMasterInterface.h"

namespace AliceO2 {
namespace Rorc {

/// A handle that refers to a page. Used by some of the ChannelMaster API calls.
class PageHandle
{
  public:
    PageHandle(int index = -1) : index(index)
    {
    }

    const int index;
};

} // namespace Rorc
} // namespace AliceO2
