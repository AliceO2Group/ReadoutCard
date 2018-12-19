/// \file Cru/Constants.h
/// \brief Definitions of internal CRU related constants
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_CONSTANTS_H_
#define ALICEO2_READOUTCARD_CRU_CONSTANTS_H_

#include "ReadoutCard/Cru.h"
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
/// Every bit represents a link. Set a bit to 0 to disable a link.
//static constexpr Register LINKS_ENABLE = 0x604;

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
/// This tells to the *dma* if the data is coming from the datapath wrapper or the internal generator
static constexpr Register DATA_SOURCE_SELECT(0x700);
static constexpr uint32_t DATA_SOURCE_SELECT_GBT = 0x0;
static constexpr uint32_t DATA_SOURCE_SELECT_INTERNAL = 0x1;

/// Register for error injection
/// Not sure how it works...
static constexpr Register DATA_GENERATOR_INJECT_ERROR(0x608);
static constexpr uint32_t DATA_GENERATOR_CONTROL_INJECT_ERROR_CMD = 0x1;

/// Reset control register
/// * Write a 1 to reset the card
/// * Write a 2 to reset data generator counter
static constexpr Register RESET_CONTROL(0x400);

/// TODO: Doesn't exist anymore
/// A debug register. The lower 8 bits of this register can be written to and read back from freely.
static constexpr Register DEBUG_READ_WRITE(0x410);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Board serial number
/// Must be accessed on BAR 2
static constexpr Register SERIAL_NUMBER(0x20002c);

/// Register containing compilation info of the firmware
/// Can be used as a sort of version number
static constexpr Register FIRMWARE_COMPILE_INFO(0x280);

/// Register containing compilation info of the firmware
/// Can be used as a sort of version number
static constexpr Register FIRMWARE_FEATURES(0x41c);

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

/// Register containing the number of dropped packets
/// Must be accessed on BAR 2
// not yet in cru_tables.py
static constexpr Register NUM_DROPPED_PACKETS(0x0060001C);

/// Registers for controlling the DDG
static constexpr Register DDG_CTRL0(0x00d00000);
static constexpr Register DDG_CTRL2(0x00d00004);

/// Register to control BSP
static constexpr Register BSP_USER_CONTROL(0x00000018);

//** TTC **//
/// Register for setting the Clock
static constexpr Register CLOCK_CONTROL(0x00240010);
static constexpr Register CTP_CLOCK(0x00240000);
static constexpr Register LOCAL_CLOCK(0x00240004);

/// Register for locking the clock the the refclk 
static constexpr Register LOCK_CLOCK_TO_REF(0x00220000);

/// Register for modifying TTC parameters (CLOCK_/DATA_)
static constexpr Register TTC_DATA(0x00200000);
/*static constexpr uint32_t CLOCK_TTC(0x0);
static constexpr uint32_t CLOCK_LOCAL(0x2);
static constexpr uint32_t DATA_CTP(0x0);
static constexpr uint32_t DATA_PATTERN(0x1);
static constexpr uint32_t DATA_MIDTRG(0x2);*/

/// Registers used for TTC calibration
static constexpr Register PON_WRAPPER_PLL(0x00224000);
static constexpr Register PON_WRAPPER_TX(0x00226000);

/// Register for configuring PON TX
static constexpr Register CLOCK_ONU_FPLL(0x00248000);
static constexpr Register CLOCK_PLL_CONTROL_ONU(0x00240018);
static constexpr Register ONU_USER_LOGIC(0x0022a000);

// Registers for setting the Ref Gen
static constexpr Register ONU_USER_REFGEN(0x0022c000);
static constexpr Register REFGEN0_OFFSET(0x00000000);
static constexpr Register REFGEN1_OFFSET(0x00000004);

//** GBT **//
/// Wrapper 0's base address
static constexpr Register WRAPPER0(0x00400000);

/// Wrapper 1's base address
static constexpr Register WRAPPER1(0x00500000);

/// GBT Wrapper configuration registers
static constexpr Register GBT_WRAPPER_BANK_OFFSET(0x00020000);
static constexpr Register GBT_BANK_LINK_OFFSET(0x00002000);
static constexpr Register GBT_LINK_REGS_OFFSET(0x00000000);

/// Register for selecting the GBT link source (i.e. Internal Data Generator)
static constexpr Register GBT_LINK_SOURCE_SELECT(0x00000038);

/// Register for selecting the the GBT Multiplexer
static constexpr Register GBT_MUX_SELECT(0x0000001c);
/*static constexpr uint32_t GBT_MUX_TTC(0x0);
static constexpr uint32_t GBT_MUX_DDG(0x1);
static constexpr uint32_t GBT_MUX_SC(0x2);*/

/// GBT registers to get Link and Wrapper parameters (e.g. count/links per bank)
static constexpr Register GBT_LINK_XCVR_OFFSET(0x00001000);
static constexpr Register GBT_WRAPPER_CLOCK_COUNTER(0x0000000c);
static constexpr Register GBT_WRAPPER_GREGS(0x00000000);
static constexpr Register GBT_WRAPPER_CONF0(0x00000000);
static constexpr Register GBT_WRAPPER_ATX_PLL(0x000e0000);
static constexpr Register GBT_BANK_FPLL(0x0000e000);

/// Registers to set TX and RX GBT modes
static constexpr Register GBT_LINK_TX_CONTROL_OFFSET(0x00000034);
static constexpr Register GBT_LINK_RX_CONTROL_OFFSET(0x00000040);
/*static constexpr uint32_t GBT_MODE_GBT(0x0);
static constexpr uint32_t GBT_MODE_WB(0x1);*/


//** DATAPATH WRAPPER **//
/// Datapath Wrapper 0 base address
static constexpr Register DWRAPPER_BASE0(0x00600000);

/// Datapath Wrapper 1 base address
static constexpr Register DWRAPPER_BASE1(0x00700000);

/// Datapath Wrapper offset for "Global Registers"
static constexpr Register DWRAPPER_GREGS(0x00000000);

/// Datapath Wrapper offset for "Enabled(?) Registers"
static constexpr Register DWRAPPER_ENREG(0x00000000);

/// Register to control the Datapath Wrapper multiplexer
static constexpr Register DWRAPPER_MUX_CONTROL(0x00000004);

/// Datapath Wrapper configuration registers
static constexpr Register DATAPATHLINK_OFFSET(0x00040000);
static constexpr Register DATALINK_OFFSET(0x00001000);
static constexpr Register DATALINK_CONTROL(0x00000000);
/*static constexpr uint32_t GBT_PACKET(0x1); 
static constexpr uint32_t GBT_CONTINUOUS(0x0);*/

/// Registers to set the Flow Control
static constexpr Register FLOW_CONTROL_OFFSET(0x000c0000);
static constexpr Register FLOW_CONTROL_REGISTER(0x00000000);

//** I2C **//
/// I2C base addresses
static constexpr Register SI5345_1(0x00030a00);
static constexpr Register SI5345_2(0x00030c00);
static constexpr Register SI5344(0x00030800);

} // namespace Cru
} // namespace Registers
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_CONSTANTS_H_
