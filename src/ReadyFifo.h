///
/// \file ReadyFifo.h
/// \author Pascal Boeschoten
///

#pragma once

#include <cstdint>

namespace AliceO2 {
namespace Rorc {

static constexpr int CRORC_NUMBER_OF_PAGES = 128;

/// Class representing CRORC readyFifo
class ReadyFifo
{
  public:
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

    std::array<Entry, CRORC_NUMBER_OF_PAGES> entries;
};

static_assert(sizeof(ReadyFifo::Entry) == 8, "Size of ReadyFifo::Entry invalid");
static_assert(sizeof(ReadyFifo) == (CRORC_NUMBER_OF_PAGES * sizeof(ReadyFifo::Entry)), "Size of ReadyFifo invalid");

} // namespace Rorc
} // namespace AliceO2
