
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file Cru/DataFormat.h
/// \brief Definitions of CRU data format functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_DATAFORMAT_H_
#define O2_READOUTCARD_SRC_DATAFORMAT_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Utilities/Util.h"

namespace o2
{
namespace roc
{
namespace DataFormat
{
namespace
{
uint32_t getWord(const char* data, int i)
{
  uint32_t word = 0;
  memcpy(&word, &data[sizeof(word) * i], sizeof(word));
  return word;
}
} // Anonymous namespace

uint32_t getLinkId(const char* data)
{
  return Utilities::getBits(getWord(data, 3), 0, 7); //bits #[96-103] from RDH word 0
}

uint32_t getMemsize(const char* data)
{
  return Utilities::getBits(getWord(data, 2), 16, 31); //bits #[80-95] from RDH word 0
}

uint32_t getPacketCounter(const char* data)
{
  return Utilities::getBits(getWord(data, 3), 8, 15); //bits #[104-111] from RDH word 0
}

uint32_t getOffset(const char* data)
{
  return Utilities::getBits(getWord(data, 2), 0, 15); //bits #[64-79] from RDH word 0
}

uint32_t getOrbit(const char* data)
{
  return Utilities::getBits(getWord(data, 5), 0, 31); //bits #[64-95] from RDH word 1
}

uint32_t getTriggerType(const char* data)
{
  return Utilities::getBits(getWord(data, 8), 0, 31); //bits #[0-31] from RDH word 3
}

uint32_t getPagesCounter(const char* data)
{
  return Utilities::getBits(getWord(data, 9), 0, 15); //bits #[40-55] from RDH word 3
}

uint32_t getBunchCrossing(const char* data)
{
  return Utilities::getBits(getWord(data, 4), 0, 31);
}

/// Get header size in bytes
constexpr size_t getHeaderSize()
{
  // Two 256-bit words = 64 bytes
  return 0x40;
}

/// Get header size in 256-bit words
constexpr size_t getHeaderSizeWords()
{
  return 2;
}

} // namespace DataFormat
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_DATAFORMAT_H_
