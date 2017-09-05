/// \file REGISTER.h
/// \brief Definition of the REGISTER struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef SRC_REGISTER_H_
#define SRC_REGISTER_H_

#include <cstdint>
#include <cstddef>

namespace AliceO2 {
namespace roc {

/// A simple struct that holds an address and index of a BAR register
struct Register
{
    constexpr Register(uintptr_t address) noexcept : address(address), index(address / 4)
    {
    }

    uintptr_t address; ///< byte-based address
    size_t index; ///< 32-bit based index
};

/// A simple struct that creates Register objects for registers that occur in intervals
struct IntervalRegister
{
    constexpr IntervalRegister(uintptr_t base, uintptr_t interval) noexcept : base(base), interval(interval)
    {
    }

    Register get(int index) const
    {
        return Register(base + interval * index);
    }

    uintptr_t base; ///< Base address of register
    uintptr_t interval; ///< Interval of register
};

} // namespace roc
} // namespace AliceO2

#endif // SRC_REGISTER_H_
