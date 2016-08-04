///
/// \file CruRegisterIndex.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <cstddef>

namespace AliceO2 {
namespace Rorc {

/// Namespace containing definitions of indexes for CRU BAR registers
/// They are based on the current best-guess understanding of both the CRU firmware and the code in here:
///   https://gitlab.cern.ch/alice-cru/pciedma_eval
/// Note that they are 32-bit indexes, not byte indexes
namespace CruRegisterIndex
{

/// Status table base address (low 32 bits)
/// Byte address: 0x0
static constexpr size_t STATUS_BASE_BUS_LOW = 0;

/// Status table base address (high 32 bits)
/// Byte address: 0x4
static constexpr size_t STATUS_BASE_BUS_HIGH = 1;

/// Destination FIFO memory address in card (low 32 bits)
/// Byte address: 0x8
static constexpr size_t FIFO_BASE_CARD_LOW = 2;

/// Destination FIFO memory address in card (high 32 bits)
/// XXX Appears to be unused, it's set to 0 in code examples
/// Byte address: 0x12
static constexpr size_t FIFO_BASE_CARD_HIGH = 3;

/// Set to number of available pages - 1
/// Byte address: 0x16
static constexpr size_t START_DMA = 4;

/// Size of the descriptor table
/// Set to the same as (number of available pages - 1)
/// Used only if descriptor table size is other than 128
/// Byte address: 0x20
static constexpr size_t DESCRIPTOR_TABLE_SIZE = 5;

/// Some kind of control register.
/// One can "Set status to send status for every page not only for the last one to write the entire status memory" by
/// writing 0x1 into this (not sure what that means).
/// (No idea what that description means)
/// Byte address: 0x24
static constexpr size_t SEND_STATUS = 6;

/// Control register for the data emulator
/// Write 0x1 to this register to enable it
/// Byte address: 0x200
static constexpr size_t DATA_EMULATOR_ENABLE = 128;

/// Control register for PCI status
/// Write 0x1 to this register to signal that the host RAM is available for transfer
/// Byte address: 0x204
static constexpr size_t PCIE_READY = 129;

/// Some kind of control register
/// One can "deassert reset for led module" by writing 0xd into this (not sure what that means).
/// This register does not appear to be necessary to use the LED on/off toggle functionality
/// Byte address: 0x220
static constexpr size_t LED_DEASSERT_RESET = 136;

/// Some kind of control register
/// One can "write data in led module" by writing 0x3 into this (not sure what that means).
/// This register does not appear to be necessary to use the LED on/off toggle functionality
/// Byte address: 0x230
static constexpr size_t LED_MODULE_DATA = 140;

/// Read status count
/// Byte address: 0x250
static constexpr size_t READ_STATUS_COUNT = 148;

/// Set to 0xff to turn the LED on, 0x00 to turn off
/// Byte address: 0x260
static constexpr size_t LED_STATUS = 152;

/// A read from this register will pop a value from the debug FIFO
/// Byte address: 0x270
static constexpr size_t DEBUG_FIFO_POP = 156;

/// A write to this register will push a value into the debug FIFO
/// Byte address: 0x274
static constexpr size_t DEBUG_FIFO_PUSH = 157;

/// Convert an index to a byte address
constexpr inline size_t toByteAddress(size_t address32)
{
  return address32 * 4;
}

} // namespace CruRegisterIndex
} // namespace Rorc
} // namespace AliceO2
