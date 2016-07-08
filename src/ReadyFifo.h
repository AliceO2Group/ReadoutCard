///
/// \file ReadyFifo.h
/// \author Pascal Boeschoten
///

#pragma once

#include <cstdint>

namespace AliceO2 {
namespace Rorc {

/// Class representing CRORC readyFifo
class ReadyFifo
{
  public:
    static constexpr int FIFO_ENTRIES = 128;

    class Entry
    {
      public:
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

    std::array<Entry, FIFO_ENTRIES> entries;
};

static_assert(sizeof(ReadyFifo::Entry) == 8, "Size of ReadyFifo::Entry invalid");
static_assert(sizeof(ReadyFifo) == (ReadyFifo::FIFO_ENTRIES * sizeof(ReadyFifo::Entry)), "Size of ReadyFifo invalid");

} // namespace Rorc
} // namespace AliceO2
