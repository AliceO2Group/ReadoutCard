/// \file CruRegisterIndex.h
/// \brief Definitions of CRU register indexes.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

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

/// Status address in card (low 32 bits)
/// Byte address: 0x8
static constexpr size_t STATUS_BASE_CARD_LOW = 2;

/// Status address in card (high 32 bits)
/// Note: Appears to be unused, it's set to 0 in code examples
/// Byte address: 0xC
static constexpr size_t STATUS_BASE_CARD_HIGH = 3;

/// Set to number of available pages - 1
/// Byte address: 0x10
[[deprecated]] static constexpr size_t DMA_POINTER = 4; // Now controlled by firmware

/// Size of the descriptor table
/// Set to the same as (number of available pages - 1)
/// Used only if descriptor table size is other than 128
/// Byte address: 0x14
static constexpr size_t DESCRIPTOR_TABLE_SIZE = 5;

/// Control register for the way the done bit is set in status registers.
/// When register bit 0 is set, the status register's done bit will be set for each descriptor and a single MSI
/// interrupt will be sent after the final descriptor completes.
/// If not set, the done bit will be set only for the final descriptor.
/// Byte address: 0x18
static constexpr size_t DONE_CONTROL = 6;

/// Control register for the data emulator
/// * bit 0: set to indicate the software is ready for DMA
/// * bit 1: set to start internal data generator
/// Byte address: 0x200
static constexpr size_t DATA_EMULATOR_CONTROL = 128;

/// Command register for DMA
/// * Write 0x1 to this register to acknowledge that the software handled a page.
///   Note that this is a "pulse" bit, not a sticky bit. It's used by the firmware to know when the software is ready to
///   accept new data
/// * Write 0x2 to inject an error
/// Byte address: 0x204
//static constexpr size_t DMA_COMMAND = 129;

/// Configuration register for DMA
/// First two bits determine the data generator pattern
///   0b01 -> Counter
///   0b10 -> 0xa5a5a5a5
///   0b11 -> 0x12345678
/// Byte address: 0x208
static constexpr size_t DMA_CONFIGURATION = 130;

/// Idle counter register lower 32 bits
/// Byte address: 0x210
static constexpr size_t IDLE_COUNTER_LOWER = 132;

/// Idle counter register upper 32 bits
/// Byte address: 0x214
static constexpr size_t IDLE_COUNTER_UPPER = 133;

///// Contains the amount of 32 kilobyte blocks that the firmware has pushed
///// Byte address: 0x218
//static constexpr size_t FIRMWARE_PUSH_COUNTER = 134;

/// Not sure
/// Byte address: 0x218
static constexpr size_t MAX_IDLE_VALUE = 134;

///// Control register for DMA FIFO pointer
///// Write the DMA pointer to this register, i.e. the index of the highest available descriptor
///// Byte address: 0x???
//static constexpr size_t FIRMWARE_DMA_POINTER = ???;

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

/// Board serial number
/// Must be accessed on BAR 2
/// Byte address: 0x20002c
static constexpr size_t SERIAL_NUMBER = 524299;

/// Register containing compilation info of the firmware
/// Can be used as a sort of version number
/// Byte address: 0x280
static constexpr size_t FIRMWARE_COMPILE_INFO = 160;

/// Reset control register
/// * Write a 1 to reset the card
/// * Write a 2 to reset data generator counter
/// Byte address: 0x290
static constexpr size_t RESET_CONTROL = 164;

/// A debug register. The lower 8 bits of this register can be written to and read back from freely.
/// Byte address: 0x310
static constexpr size_t DEBUG_READ_WRITE = 196;

/// Temperature control & read register
/// Must be accessed on BAR 2
/// The lower 10 bits contain the temperature value
/// Byte address: 0x200028
static constexpr size_t TEMPERATURE = 524298;

/// Convert an index to a byte address
constexpr inline size_t toByteAddress(size_t address32)
{
  return address32 * 4;
}

} // namespace CruRegisterIndex
} // namespace Rorc
} // namespace AliceO2
