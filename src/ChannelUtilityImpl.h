///
/// \file ChannelUtilityImpl.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Implementations of the ChannelUtilityInterface functions. The functions here are called by the ChannelMaster
///   objects. While the ChannelMaster inherits directly from the ChannelUtilityInterface, we don't want to pollute them
///   with these sorts of functions.
///

#pragma once

#include <vector>
#include <ostream>
#include "ReadyFifo.h"
#include "CruFifoTable.h"

namespace AliceO2 {
namespace Rorc {
namespace ChannelUtility {

void printCrorcFifo(ReadyFifo* fifo, std::ostream& os);
void printCruFifo(CruFifoTable* fifo, std::ostream& os);

} // namespace ChannelUtility
} // namespace Rorc
} // namespace AliceO2
