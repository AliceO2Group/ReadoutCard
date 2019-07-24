// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Swt.h
/// \brief Definition of SWT operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_UTILITIES_SWT_H
#define ALICEO2_READOUTCARD_UTILITIES_SWT_H

#include <string>
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "Swt/SwtWord.h"

namespace AliceO2
{
namespace roc
{

#define SWT_WRITE_BAR_WRITES 5
#define SWT_WRITE_BAR_READS 1
#define SWT_READ_BAR_WRITES 2
#define SWT_READ_BAR_READS 4

/// Class for Single Word Transactions with the CRU
class Swt
{
 public:
  Swt(RegisterReadWriteInterface& bar2, int gbtChannel);

  void reset();
  uint32_t write(SwtWord& swtWord);
  uint32_t read(SwtWord& swtWord);

 private:
  void setChannel(int gbtChannel);
  void barWrite(uint32_t offset, uint32_t data);
  uint32_t barRead(uint32_t index);

  RegisterReadWriteInterface& mBar2;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_UTILITIES_SWT_H
