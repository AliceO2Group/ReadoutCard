/// \file Util.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <chrono>
#include <cstdint>
#include <boost/lexical_cast.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Utilities {

template <typename T1, typename T2>
void lexicalCast(const T1& from, T2& to)
{
  to = boost::lexical_cast<T2>(from);
}

inline uint32_t getLower32Bits(uint64_t x)
{
  return x & 0xFfffFfff;
}

inline uint32_t getUpper32Bits(uint64_t x)
{
  return (x >> 32) & 0xFfffFfff;
}

template <typename T>
T getBits(T x, int lsb, int msb) {
  assert(lsb < msb);
  return (x >> lsb) & ~(~0 << (msb-lsb+1));
};

/// Offset a pointer by a given amount of bytes
template <typename T>
T* offsetBytes(T* pointer, size_t bytes)
{
  return reinterpret_cast<T*>(reinterpret_cast<char*>(pointer) + bytes);
}

/// Calculate difference in bytes between two pointers
template <typename T>
ptrdiff_t pointerDiff(T* a, T* b)
{
  return reinterpret_cast<char*>(a) - reinterpret_cast<char*>(b);
}

/// Get a range from std::rand()
inline int getRandRange(int min, int max)
{
  return (std::rand() % max - min) + min;
}

/// Wait on a given condition to become true, with a timeout. Uses a busy wait
/// \return false on timeout, true if the condition was satisfied before a timeout occurred
template <typename Predicate, typename Duration>
bool waitOnPredicateWithTimeout(Duration duration, Predicate predicate)
{
  auto start = std::chrono::high_resolution_clock::now();
  while (predicate() == false) {
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - start) > duration) {
      return false;
    }
  }
  return true;
}

inline bool checkAlignment(void* address, uint64_t alignment)
{
  return (uint64_t(address) % alignment) == 0;
}

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
