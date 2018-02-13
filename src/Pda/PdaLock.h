/// \file PdaDmaBuffer.h
/// \brief Definition of the PdaDmaBuffer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_PDA_PDALOCK_H_
#define ALICEO2_SRC_READOUTCARD_PDA_PDALOCK_H_

#include "InterprocessLock.h"

namespace AliceO2 {
namespace roc {
namespace Pda {

/// Represents a global, system-wide lock on ReadoutCard's PDA usage. This is needed because the PDA kernel module
/// will lock up if buffers are created/freed in parallel.
/// Just hope nobody else uses PDA in parallel.
class PdaLock
{
  public:

    /// Be careful you don't use it like this:
    ///   Pda::PdaLock lock()
    /// But rather like this:
    ///   Pda::PdaLock lock{}
    PdaLock(bool wait = true) : mLock("/dev/shm/AliceO2_RoC_Pda.lock", "AliceO2_RoC_Pda_Mutex", wait)
    {
    }

  private:
    Interprocess::Lock mLock;
};

} // namespace Pda
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_PDA_PDALOCK_H_