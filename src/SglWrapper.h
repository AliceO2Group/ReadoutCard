///
/// \file SglWrapper.h
/// \author Pascal Boeschoten
///

#pragma once

#include <vector>
#include "pda.h"

namespace AliceO2 {
namespace Rorc {

/// Wrapper around the PDA scatter-gather list object
/// TODO Make more private
class SglWrapper
{
  public:
    class Page
    {
      public:
        inline Page(DMABuffer_SGNode* node, int pageOffset) :
          userAddress(static_cast<volatile void*>(static_cast<volatile char*>(node->u_pointer) + pageOffset)),
          busAddress(static_cast<volatile void*>(static_cast<volatile char*>(node->d_pointer) + pageOffset))
        {
        }

        volatile void* const userAddress;
        volatile void* const busAddress;
    };

    SglWrapper(DMABuffer_SGNode* startNode, size_t pageSize, size_t fullOffset, int pageCount);

    std::vector<DMABuffer_SGNode*> nodes;
    std::vector<Page> pages;
};

} // namespace Rorc
} // namespace AliceO2
