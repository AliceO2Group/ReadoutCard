///
/// \file ReadyFifo.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <cstdint>
#include <array>

namespace AliceO2 {
namespace Rorc {

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
        volatile int32_t length;
        volatile int32_t status;

        inline void reset()
        {
          length = -1;
          status = -1;
        }
    };

    inline void reset()
    {
      for (auto& e : entries) {
        e.reset();
      }
    }

    std::array<Entry, READYFIFO_ENTRIES> entries;
    std::array<int32_t, READYFIFO_ENTRIES * 2> dataInt32;
    std::array<char, READYFIFO_ENTRIES * sizeof(Entry)> dataChar;
};

// These asserts are to check if the ReadyFifo struct is the expected size, and is not being padded or something like
// that. The size is critical, because the structure must map exactly to what the C-RORC expects
static_assert(sizeof(ReadyFifo::Entry) == 8, "Size of ReadyFifo::Entry invalid");
static_assert(sizeof(ReadyFifo) == (READYFIFO_ENTRIES * sizeof(ReadyFifo::Entry)), "Size of ReadyFifo invalid");

} // namespace Rorc
} // namespace AliceO2
