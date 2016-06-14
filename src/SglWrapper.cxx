///
/// \file SglWrapper.cxx
/// \author Pascal Boeschoten
///

#include "SglWrapper.h"
#include <iostream>
#include <iomanip>

namespace AliceO2 {
namespace Rorc {

SglWrapper::SglWrapper(DMABuffer_SGNode* startNode, size_t pageSize, size_t fullOffset, int pageCount)
{
  if (startNode == nullptr) {
    return;
  }

  // Init nodes vector
  auto node = startNode;
  while (node != nullptr) {
    nodes.push_back(node);
    node = node->next;
  }

  // Init pages vector
  for (int i = 0; i < nodes.size(); ++i) {
    auto node = nodes[i];
    int baseOffset = 0;
    size_t spaceLeft = node->length;

    if (i == 0) {
      // First node is a special case, it might contain no data
      if (node->length < fullOffset + pageSize) {
        // Node is too small, data starts at second node
        continue;
      } else {
        // Node has enough room for at least one page
        baseOffset = fullOffset;
        spaceLeft -= fullOffset;
      }
    }

    int amountOfPages = spaceLeft / pageSize;

    for (int j = 0; j < amountOfPages; ++j) {
      // In some cases, the buffer may be large enough to contain an excessive amount of pages
      // We don't need to keep track of the excess pages
      if (j >= pageCount) {
        break;
      }

      int pageOffset = baseOffset + j * pageSize;
//      std::cout << "Page " << std::hex
//          << " usr " << std::setw(14) << Page(node,pageOffset).userAddress
//          << " bus " << std::setw(14) << Page(node,pageOffset).busAddress
//          << " offset " << std::setw(14) << pageOffset
//          << std::dec << std::endl;
      pages.push_back(Page(node, pageOffset));
    }
  }
}

} // namespace Rorc
} // namespace AliceO2
