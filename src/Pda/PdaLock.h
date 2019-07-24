// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file PdaDmaBuffer.h
/// \brief Definition of the PdaDmaBuffer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_PDA_PDALOCK_H_
#define ALICEO2_SRC_READOUTCARD_PDA_PDALOCK_H_

#include "ReadoutCard/InterprocessLock.h"

namespace AliceO2
{
namespace roc
{
namespace Pda
{

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
  PdaLock(bool waitOnLock = true) : mLock("Alice_O2_RoC_PDA_lock", waitOnLock)
  {
  }

  ~PdaLock()
  {
  }

 private:
  Interprocess::Lock mLock;
};

} // namespace Pda
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_PDA_PDALOCK_H_
