/// \file Cru/Constants.h
/// \brief Definitions of CRU related constants
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_CONSTANTS_H_
#define ALICEO2_READOUTCARD_CRU_CONSTANTS_H_

#include "Register.h"

namespace AliceO2
{
namespace roc
{
namespace Cru
{

/// Maximum amount of available links
static constexpr int MAX_LINKS = 32;

/// Amount of available superpage descriptors per link
static constexpr int MAX_SUPERPAGE_DESCRIPTORS = 128;

/// DMA page length in bytes
/// Note: the CRU has a firmware defined fixed page size
static constexpr size_t DMA_PAGE_SIZE = 8 * 1024;

namespace Registers
{

///*** bar0 ***///

/// Control register for the data emulator
/// * bit 0: Flow control
static constexpr Register DMA_CONTROL(0x200);

/// Interval for link BAR addresses
static constexpr uintptr_t LINK_INTERVAL = 0x10;

/// High address of superpage
static constexpr IntervalRegister LINK_SUPERPAGE_ADDRESS_HIGH(0x204, LINK_INTERVAL);

/// Low address of superpage
static constexpr IntervalRegister LINK_SUPERPAGE_ADDRESS_LOW(0x208, LINK_INTERVAL);

/// Size of the superpage in 8KiB pages
static constexpr IntervalRegister LINK_SUPERPAGE_SIZE(0x20c, LINK_INTERVAL);

/// Interval for SUPERPAGES_PUSHED addresses
static constexpr uintptr_t SUPERPAGES_PUSHED_INTERVAL = 0x4;

/// Amount of completely pushed superpages
static constexpr IntervalRegister LINK_SUPERPAGES_PUSHED(0x800, SUPERPAGES_PUSHED_INTERVAL);

/// Enable/disable links
/// Every bit represents a link. Set a bit to 1 to disable a link.
static constexpr Register LINKS_ENABLE = 0x604;

/// Configuration register for data generator
/// Bit 0: set to start data generator
/// Bits [2:1] determine the data generator pattern
///   0b01 -> Counter
///   0b10 -> 0xa5a5a5a5
///   0b11 -> 0x12345678
/// Bit 3: set to inject error
static constexpr Register DATA_GENERATOR_CONTROL(0x600);

/// Selection of data source
/// 0x0 -> GBT
/// 0x1 -> Internal data generator
/// 0x2 -> Pattern generator
static constexpr Register DATA_SOURCE_SELECT(0x700);
static constexpr uint32_t DATA_SOURCE_SELECT_GBT = 0x0;
static constexpr uint32_t DATA_SOURCE_SELECT_INTERNAL = 0x1;
static constexpr uint32_t DATA_SOURCE_SELECT_PATTERN = 0x2;

/// Register for error injection
/// Not sure how it works...
static constexpr Register DATA_GENERATOR_INJECT_ERROR(0x608);
static constexpr uint32_t DATA_GENERATOR_CONTROL_INJECT_ERROR_CMD = 0x1;

/// Board serial number
/// Must be accessed on BAR 2
static constexpr Register SERIAL_NUMBER(0x20002c);

/// Register containing compilation info of the firmware
/// Can be used as a sort of version number
static constexpr Register FIRMWARE_COMPILE_INFO(0x280);

/// Register containing compilation info of the firmware
/// Can be used as a sort of version number
static constexpr Register FIRMWARE_FEATURES(0x41c);

/// Reset control register
/// * Write a 1 to reset the card
/// * Write a 2 to reset data generator counter
static constexpr Register RESET_CONTROL(0x400);

/// A debug register. The lower 8 bits of this register can be written to and read back from freely.
static constexpr Register DEBUG_READ_WRITE(0x410);


///*** bar2 ***///

/// Temperature control & read register
/// The lower 10 bits contain the temperature value
/// Must be accessed on BAR 2
static constexpr Register TEMPERATURE(0x00010008);

/// Register containing the firmware Git hash
/// Must be accessed on BAR 2
static constexpr Register FIRMWARE_GIT_HASH(0x4);

/// Register containing the compilation date/time in seconds since Unix epoch
/// Must be accessed on BAR 2
static constexpr Register FIRMWARE_EPOCH(0x1c);

/// Register containing the compilation date/time in seconds since Unix epoch
/// Must be accessed on BAR 2
static constexpr Register FIRMWARE_DATE(0x00000008);

/// Register containing the compilation date/time in seconds since Unix epoch
/// Must be accessed on BAR 2
static constexpr Register FIRMWARE_TIME(0x0000000c);

/// Register containing the first part of the Arria 10 chip ID
/// Must be accessed on BAR 2
static constexpr Register FPGA_CHIP_HIGH(0x00010014);

/// Register containing the second part of the Arria 10 chip ID
/// Must be accessed on BAR 2
static constexpr Register FPGA_CHIP_LOW(0x00010018);

// Register containing the number of dropped packets
// Must be accessed on BAR 2
//not yet in cru_tables.py//
static constexpr Register NUM_DROPPED_PACKETS(0x0060001C);

// Register containing the CTP Clock's value
// Must be accessed on BAR 2
static constexpr Register CTP_CLOCK(0x00240000);

// Register containing the Local Clock's value
// Must be accessed on BAR 2
static constexpr Register LOCAL_CLOCK(0x00240004);

// Wrapper 0's base address
// Must be accessed on BAR 2
static constexpr Register WRAPPER0(0x00400000);

// Wrapper 1's base address
// Must be accessed on BAR 2
static constexpr Register WRAPPER1(0x00500000);

} // namespace Cru
} // namespace Registers
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_CONSTANTS_H_
