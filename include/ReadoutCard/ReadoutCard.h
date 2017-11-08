/// \file ReadoutCard.h
/// \brief Convenience header and definitions of global functions for the ReadoutCard module
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/CardType.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/DmaChannelInterface.h"
#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"

namespace AliceO2 {
namespace roc {

/// Optional pre-initialization of the driver. Also calls freeUnusedChannelBuffers().
void initializeDriver();

/// A crash may leave a channel buffer registered with PDA, which then keeps its shared-memory file handle open in the
/// kernel module. If another channel buffer is registered with the same channel, this old one will be cleaned up
/// automatically by the driver. However, in memory-constrained environments, it may not be possible to allocate a new
/// channel buffer. In such cases, this function will come in handy.
void freeUnusedChannelBuffers();

} // namespace roc
} // namespace AliceO2
