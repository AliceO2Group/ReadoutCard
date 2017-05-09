/// \file Cru/Constants.h
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
/// Control register for the data emulator
/// * bit 0: Flow control
static constexpr size_t DMA_CONTROL = 0x200/4;

/// High address of superpage
static constexpr int SUPERPAGE_ADDRESS_HIGH = 0x204/4;

/// Low address of superpage
static constexpr int SUPERPAGE_ADDRESS_LOW = 0x208/4;

/// Size of the superpage in 8KiB pages
static constexpr int SUPERPAGE_PAGES_AVAILABLE = 0x20c/4;

/// Status of superpages
/// Byte address: 0x210
static constexpr int SUPERPAGE_STATUS = 0x210/4;

/// Amount of completely pushed superpages
static constexpr int SUPERPAGES_PUSHED = 0x240/4;

/// Configuration register for data generator
/// Bit 0: set to start data generator
/// Bits [2:1] determine the data generator pattern
///   0b01 -> Counter
///   0b10 -> 0xa5a5a5a5
///   0b11 -> 0x12345678
/// Bit 3: set to inject error
static constexpr size_t DATA_GENERATOR_CONTROL = 0x320/4;

/// Some kind of control register
/// One can "deassert reset for led module" by writing 0xd into this (not sure what that means).
/// This register does not appear to be necessary to use the LED on/off toggle functionality
/// Byte address: 0x220
[[deprecated]] static constexpr size_t LED_DEASSERT_RESET = 136;

/// Some kind of control register
/// One can "write data in led module" by writing 0x3 into this (not sure what that means).
/// This register does not appear to be necessary to use the LED on/off toggle functionality
/// Byte address: 0x230
[[deprecated]] static constexpr size_t LED_MODULE_DATA = 140;

/// Set to 0xff to turn the LED on, 0x00 to turn off
/// Byte address: 0x260
[[deprecated]] static constexpr size_t LED_STATUS = 152;

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
static constexpr size_t RESET_CONTROL = 0x300/4;

/// A debug register. The lower 8 bits of this register can be written to and read back from freely.
static constexpr size_t DEBUG_READ_WRITE = 0x310/4;

/// Temperature control & read register
/// Must be accessed on BAR 2
/// The lower 10 bits contain the temperature value
/// Byte address: 0x200028
static constexpr size_t TEMPERATURE = 524298;

/// Register containing the firmware Git hash
/// Must be accessed on BAR 2
/// Byte address: 0x4
static constexpr size_t FIRMWARE_GIT_HASH = 1;

/// Register containing the compilation date/time in seconds since Unix epoch
/// Must be accessed on BAR 2
/// Byte address: 0x1c
static constexpr size_t FIRMWARE_EPOCH = 7;

/// Register containing the compilation date/time in seconds since Unix epoch
/// Must be accessed on BAR 2
/// Byte address: 0x20
static constexpr size_t FIRMWARE_DATE = 8;

/// Register containing the compilation date/time in seconds since Unix epoch
/// Must be accessed on BAR 2
/// Byte address: 0x24
static constexpr size_t FIRMWARE_TIME = 9;

} // namespace Cru
} // namespace Registers
} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_RORC_CRU_CONSTANTS
