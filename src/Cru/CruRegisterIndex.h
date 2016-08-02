///
/// \file CruRegisterIndex.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <cstddef>

namespace AliceO2 {
namespace Rorc {
/// Namespace containing definitions of indexes for CRU BAR registers
namespace CruRegisterIndex
{

/// Status table base address (low 32 bits)
static constexpr size_t STATUS_BASE_BUS_LOW = 0;

/// Status table base address (high 32 bits)
static constexpr size_t STATUS_BASE_BUS_HIGH = 1;

/// Destination FIFO memory address in card (low 32 bits)
static constexpr size_t FIFO_BASE_CARD_LOW = 2;

/// Destination FIFO memory address in card (high 32 bits)
/// XXX Appears to be unused, it's set to 0 in code examples
static constexpr size_t FIFO_BASE_CARD_HIGH = 3;

/// Set to number of available pages - 1
static constexpr size_t START_DMA = 4;

/// Size of the descriptor table
/// Set to the same as (number of available pages - 1)
/// Used only if descriptor table size is other than 128
static constexpr size_t DESCRIPTOR_TABLE_SIZE = 5;

/// Set status to send status for every page not only for the last one to write the entire status memory
/// (No idea what that description means)
static constexpr size_t SEND_STATUS = 6;

/// Enable data emulator
static constexpr size_t DATA_EMULATOR_ENABLE = 128;

/// Signals that the host RAM is available for transfer
static constexpr size_t PCIE_READY = 129;

/// Set to 0xff to turn the LED on, 0x00 to turn off
static constexpr size_t LED_STATUS = 0x260 / 4;

static constexpr size_t DEBUG_FIFO_PUSH = 0x274 / 4;
static constexpr size_t DEBUG_FIFO_POP = 0x270 / 4;

} // namespace CruRegisterIndex
} // namespace Rorc
} // namespace AliceO2
