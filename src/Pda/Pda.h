/// \file Pda.h
/// \brief Definition of PDA related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef SRC_PDA_PDA_H_
#define SRC_PDA_PDA_H_

#include <vector>
#include "ExceptionInternal.h"
#include "PageAddress.h"
#include "PdaDmaBuffer.h"

namespace AliceO2 {
namespace roc {
namespace Pda {

/// Partition the memory of the scatter gather list into sections for the FIFO and pages
/// \param list Scatter gather list
/// \param fifoSize Size of the FIFO
/// \param pageSize Size of a page
/// \return A tuple containing: address of the page for the FIFO, and a vector with addresses of the rest of the pages
std::tuple<PageAddress, std::vector<PageAddress>> partitionScatterGatherList(
    const Pda::PdaDmaBuffer::ScatterGatherVector& list, size_t fifoSize, size_t pageSize);

} // namespace Pda
} // namespace roc
} // namespace AliceO2

#endif /* SRC_PDA_PDA_H_ */
