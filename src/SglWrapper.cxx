///
/// \file SglWrapper.cxx
/// \author Pascal Boeschoten
///

#include "SglWrapper.h"
#include <iostream>
#include <iomanip>
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

using std::cout;
using std::endl;

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

    // The first node is a special case. It contains the readyFifo, which might prevent a page from fitting in
    // as well. In that case, pages start at the second node
    if (i == 0) {
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
      if (pages.size() >= pageCount) {
        cout << "AliceO2::Rorc::SglWrapper::SglWrapper() -> Warning: excessive amount of pages" << endl;
        break;
      }

      int pageOffset = baseOffset + j * pageSize;
      pages.push_back(Page(node, pageOffset));
    }

    if (pages.size() >= pageCount) {
      break;
    }
  }

  if (pages.size() < pageCount) {
    cout << "Scatter-gather list could not fit enough pages: " << pages.size() << " out of " << pageCount << endl;
    ALICEO2_RORC_THROW_EXCEPTION("Scatter-gather list could not fit enough pages");
  }
}

} // namespace Rorc
} // namespace AliceO2
