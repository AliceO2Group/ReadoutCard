/// \file Util.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstdint>

namespace AliceO2 {
namespace roc {
namespace Utilities {

/// Is x a multiple of y
template <typename T1, typename T2>
bool isMultiple(const T1& x, const T2& y)
{
  return (x >= y) && ((x % y) == 0);
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
  return (x >> lsb) & ~(~T(0) << (msb - lsb + T(1)));
}

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

inline bool checkAlignment(void* address, uint64_t alignment)
{
  return (uint64_t(address) % alignment) == 0;
}

} // namespace Util
} // namespace roc
} // namespace AliceO2
