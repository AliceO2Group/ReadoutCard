/// \file PdaDmaBuffer.h
/// \brief Definition of the PDA related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef SRC_PDA_PDA_H_
#define SRC_PDA_PDA_H_

#include "ExceptionInternal.h"
#include "PageAddress.h"
#include "PdaDmaBuffer.h"

namespace AliceO2 {
namespace Rorc {
namespace Pda {

/// Partition the memory of the scatter gather list into sections for the FIFO and pages
/// \param list Scatter gather list
/// \param fifoSize Size of the FIFO
/// \param pageSize Size of a page
/// \return A tuple containing: address of the page for the FIFO, and a vector with addresses of the rest of the pages
inline std::tuple<PageAddress, std::vector<PageAddress>> partitionScatterGatherList(
    const Pda::PdaDmaBuffer::ScatterGatherVector& list, size_t fifoSize, size_t pageSize)
{
  std::tuple<PageAddress, std::vector<PageAddress>> tuple;
  auto& fifoAddress = std::get<0>(tuple);
  auto& pageAddresses = std::get<1>(tuple);

  if (list.size() < 1) {
    BOOST_THROW_EXCEPTION(CruException()
        << ErrorInfo::Message("Scatter-gather list empty"));
  }

  if (list.at(0).size < fifoSize) {
    BOOST_THROW_EXCEPTION(CruException()
        << ErrorInfo::Message("First SGL entry size insufficient for FIFO"));
  }

  fifoAddress = {list.at(0).addressUser, list.at(0).addressBus};

  for (int i = 0; i < list.size(); ++i) {
    auto& entry = list[i];
    if (entry.size < (2l * 1024l * 1024l)) {
      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("Unsupported configuration: DMA scatter-gather entry size less than 2 MiB")
          << ErrorInfo::ScatterGatherEntrySize(entry.size)
          << ErrorInfo::PossibleCauses(
              {"DMA buffer was not allocated in hugepage shared memory (hugetlbfs may not be properly mounted)"}));
    }

    bool first = (i == 0);

    // How many pages fit in this SGL entry
    int64_t pagesInSglEntry = first
        ? (entry.size - fifoSize) / pageSize // First entry also contains the FIFO
        : entry.size / pageSize; // Otherwise, we can fill it up with pages

    for (int64_t j = 0; j < pagesInSglEntry; ++j) {
      int64_t offset = first
        ? fifoSize + j * pageSize// Keep that FIFO in mind...
        : j * pageSize;

      pageAddresses.push_back({Util::offsetBytes(entry.addressUser, offset),
        Util::offsetBytes(entry.addressBus, offset)});
    }
  }

  return tuple;
}

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2

#endif /* SRC_PDA_PDA_H_ */
