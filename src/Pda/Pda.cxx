/// \file Pda.cxx
/// \brief Implementation of PDA related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Pda/Pda.h"
#include "ExceptionInternal.h"
#include "PageAddress.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {
namespace Pda {

std::tuple<PageAddress, std::vector<PageAddress>> partitionScatterGatherList(
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

  for (size_t i = 0; i < list.size(); ++i) {
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

      pageAddresses.push_back({entry.addressUser + offset, entry.addressBus + offset});
    }
  }

  return tuple;
}

} // namespace Pda
} // namespace roc
} // namespace AliceO2
