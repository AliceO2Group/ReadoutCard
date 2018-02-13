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
    PdaLock() : mLock("/dev/shm/alice_o2/rorc/AliceO2_roc_Pda_PdaDmaBuffer.lock", "AliceO2_roc_Pda_PdaDmaBuffer_Mutex",
      true)
    {
    }

  private:
    Interprocess::Lock mLock;
};

} // namespace Pda
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_PDA_PDALOCK_H_