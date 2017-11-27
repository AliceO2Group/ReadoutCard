/// \file Register.h
/// \brief Definition of the Register struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_REGISTER_H_
#define ALICEO2_SRC_READOUTCARD_REGISTER_H_

#include <cstdint>
#include <cstddef>

namespace AliceO2 {
namespace roc {

/// A simple struct that holds an address and 32-bit index of a BAR register
/// This is convenient because:
///  - We generally program with the index
///  - We'd like to initialize it with the address (the CRU people think in addresses and it makes it easier to check)
///  - We occasionally need the address
struct Register
{
    /// \param address Address of the register
    constexpr Register(uintptr_t address) noexcept : address(address), index(address / 4)
    {
    }

    const uintptr_t address; ///< byte-based address
    const size_t index; ///< 32-bit based index
};

/// A simple struct that creates Register objects for registers that occur in intervals
/// For example, the CRU has repeating registers for the multiple links. This allows us to describe that conveniently.
struct IntervalRegister
{
    /// \param base Base address of the interval register
    /// \param interval Interval of the register
    constexpr IntervalRegister(uintptr_t base, uintptr_t interval) noexcept : base(base), interval(interval)
    {
    }

    /// \param index of the register
    Register get(int index) const
    {
        return Register(base + interval * index);
    }

    const uintptr_t base; ///< Base address of register
    const uintptr_t interval; ///< Interval of register
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_REGISTER_H_
