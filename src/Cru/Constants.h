/// \file Constants.h
/// \brief Definitions of CRU related constants
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_RORC_CRU_CONSTANTS
#define ALICEO2_RORC_CRU_CONSTANTS

namespace AliceO2
{
namespace Rorc
{
namespace Cru
{

/// Amount of available superpage descriptors
static constexpr int SUPERPAGE_DESCRIPTORS = 32;

namespace Registers
{
/// High address of superpage
/// Byte address: 0x210
static constexpr int SUPERPAGE_ADDRESS_HIGH = 132;

/// Low address of superpage
/// Byte address: 0x214
static constexpr int SUPERPAGE_ADDRESS_LOW = 133;

/// Pages available and index of superpage
/// Bits [0:4] index
/// Bites[5:31] 8KiB pages available in superpages
/// Byte address: 0x234
static constexpr int SUPERPAGE_PAGES_AVAILABLE_AND_INDEX = 141;

/// Status of superpages
/// Byte address: 0x23c
static constexpr int SUPERPAGE_STATUS = 0x23c;

/// Amount of pushed pages in superpage of index 0.
/// Byte address: 0x240
static constexpr int SUPERPAGE_PUSHED_PAGES = 0x240;

/// Control register for the data emulator
/// * bit 0: set to indicate the software is ready for DMA
/// * bit 1: set to start internal data generator
/// Byte address: 0x200
static constexpr size_t DATA_EMULATOR_CONTROL = 128;

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

/// Not sure
/// Byte address: 0x218
static constexpr size_t MAX_IDLE_VALUE = 134;

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
/// Byte address: 0x400
static constexpr size_t RESET_CONTROL = 256;

/// A debug register. The lower 8 bits of this register can be written to and read back from freely.
/// Byte address: 0x410
static constexpr size_t DEBUG_READ_WRITE = 260;

/// Temperature control & read register
/// Must be accessed on BAR 2
/// The lower 10 bits contain the temperature value
/// Byte address: 0x200028
static constexpr size_t TEMPERATURE = 524298;

} // namespace Cru
} // namespace Registers
} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_RORC_CRU_CONSTANTS
