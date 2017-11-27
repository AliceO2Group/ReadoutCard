/// \file Driver.h
/// \brief Definitions of global functions for the ReadoutCard module
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_DRIVER_H_
#define ALICEO2_INCLUDE_READOUTCARD_DRIVER_H_

namespace AliceO2 {
namespace roc {
namespace driver {

/// Optional pre-initialization of the driver. Also calls freeUnusedChannelBuffers().
void initialize();

/// A crash may leave a channel buffer registered with PDA, which then keeps its shared-memory file handle open in the
/// kernel module. If another channel buffer is registered with the same channel, this old one will be cleaned up
/// automatically by the driver. However, in memory-constrained environments, it may not be possible to allocate a new
/// channel buffer. In such cases, this function will come in handy.
/// Note that root or pda group permissions are required.
void freeUnusedChannelBuffers();

} // namespace driver
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_DRIVER_H_
