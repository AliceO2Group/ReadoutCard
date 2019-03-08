// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ReadyFifo.h
/// \brief Definition of the ReadyFifo class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_CRORC_READYFIFO_H_
#define ALICEO2_SRC_READOUTCARD_CRORC_READYFIFO_H_

#include <cstdint>
#include <array>

namespace AliceO2 {
namespace roc {

constexpr int READYFIFO_ENTRIES = 128;

/// Class representing CRORC readyFifo
/// This class is meant to be used as an aliased type, reinterpret_casted from a raw memory pointer
/// Since this is an aggregate type, it does not violate the strict aliasing rule.
/// For more information, see:
///   http://en.cppreference.com/w/cpp/language/reinterpret_cast
///   http://en.cppreference.com/w/cpp/language/aggregate_initialization
union ReadyFifo
{
    struct Entry
    {
        volatile int32_t length; ///< Length of the received page in 32-bit words
        volatile int32_t status; ///< Status of the received page

        void reset()
        {
          length = -1;
          status = -1;
        }

        uint32_t getSize()
        {
          return length * 4;
        }
    };

    void reset()
    {
      for (auto& e : entries) {
        e.reset();
      }
    }

    std::array<Entry, READYFIFO_ENTRIES> entries;
    std::array<volatile int32_t, READYFIFO_ENTRIES * 2> dataInt32;
    std::array<volatile char, READYFIFO_ENTRIES * sizeof(Entry)> dataChar;
};

// These asserts are to check if the ReadyFifo struct is the expected size, and is not being padded or something like
// that. The size is critical, because the structure must map exactly to what the C-RORC expects
static_assert(sizeof(ReadyFifo::Entry) == 8, "Size of ReadyFifo::Entry invalid");
static_assert(sizeof(ReadyFifo) == (READYFIFO_ENTRIES * sizeof(ReadyFifo::Entry)), "Size of ReadyFifo invalid");

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_CRORC_READYFIFO_H_
