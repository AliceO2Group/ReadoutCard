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
#include "Crorc/ReadyFifo.h"
#include "Cru/CruFifoTable.h"
#include "RORC/RegisterReadWriteInterface.h"
#include "ChannelPaths.h"

namespace AliceO2 {
namespace Rorc {
namespace ChannelUtility {

void printCrorcFifo(ReadyFifo* fifo, std::ostream& os);
void printCruFifo(CruFifoTable* fifo, std::ostream& os);

void crorcSanityCheck(std::ostream& os, RegisterReadWriteInterface* channel);
void cruSanityCheck(std::ostream& os, RegisterReadWriteInterface* channel);

void crorcCleanupState(const ChannelPaths& paths);
void cruCleanupState(const ChannelPaths& paths);

} // namespace ChannelUtility
} // namespace Rorc
} // namespace AliceO2
