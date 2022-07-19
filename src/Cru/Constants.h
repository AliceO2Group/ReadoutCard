
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file Cru/Constants.h
/// \brief Definitions of internal CRU related constants
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_CONSTANTS_H_
#define O2_READOUTCARD_CRU_CONSTANTS_H_

#include "ReadoutCard/Cru.h"

namespace o2
{
namespace roc
{
namespace Cru
{

/// Maximum amount of available links
static constexpr int MAX_LINKS = 16;

/// Amount of available superpage descriptors per link
static constexpr int MAX_SUPERPAGE_DESCRIPTORS_DEFAULT = 128;

/// DMA page length in bytes
/// Note: the CRU has a firmware defined fixed page size
static constexpr size_t DMA_PAGE_SIZE = 8 * 1024;

namespace Registers
{

///*** bar0 ***///

/// Control register for the data emulator
/// * bit 0: Flow control
static constexpr Register DMA_CONTROL(0x00000200);

/// Link interval for superpage addresses to push
static constexpr uintptr_t LINK_INTERVAL = 0x10;

/// High address of superpage
static constexpr IntervalRegister LINK_SUPERPAGE_ADDRESS_HIGH(0x00000204, LINK_INTERVAL);

/// Low address of superpage
static constexpr IntervalRegister LINK_SUPERPAGE_ADDRESS_LOW(0x00000208, LINK_INTERVAL);

/// Size of the superpage in 8KiB pages
static constexpr IntervalRegister LINK_SUPERPAGE_PAGES(0x0000020c, LINK_INTERVAL);

/// Link interval for superpage ready addresses
static constexpr uintptr_t SUPERPAGES_READY_INTERVAL = 0x4;

/// Amount of ready superpages
static constexpr IntervalRegister LINK_SUPERPAGE_COUNT(0x00000800, SUPERPAGES_READY_INTERVAL);

// FIFO containing the size of the ready superpages
static constexpr IntervalRegister LINK_SUPERPAGE_SIZE(0x00000840, SUPERPAGES_READY_INTERVAL);

// Counter for the times a link's Superpage FIFO is empty
static constexpr IntervalRegister LINK_SUPERPAGE_FIFO_EMPTY(0x00000880, SUPERPAGES_READY_INTERVAL);

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
static constexpr Register DATA_GENERATOR_CONTROL(0x00000600);

/// Register for error injection
/// Not sure how it works...
static constexpr Register DATA_GENERATOR_INJECT_ERROR(0x00000608);
static constexpr uint32_t DATA_GENERATOR_CONTROL_INJECT_ERROR_CMD = 0x1;

/// Selection of data source
/// 0x0 -> GBT
/// 0x1 -> Internal data generator
/// This tells to the *dma* if the data is coming from the datapath wrapper or the internal generator
static constexpr Register DATA_SOURCE_SELECT(0x00000700);
static constexpr uint32_t DATA_SOURCE_SELECT_GBT = 0x0;
static constexpr uint32_t DATA_SOURCE_SELECT_INTERNAL = 0x1;

/// Reset control register
/// * Write a 1 to reset the card
/// * Write a 2 to reset data generator counter
static constexpr Register RESET_CONTROL(0x00000400);

/// Debug register
/// * Write 0x2 to set debug mode
/// * Write 0x0 to unset
static constexpr Register DEBUG(0x00000c00);

/// Register from which the CRUs endpoint number is available
/// 0x0        -> Endpoint #0
/// 0x11111111 -> Endpoint #1
static constexpr Register ENDPOINT_ID(0x00000500);

/// Register to get the size of the internal CRU per-link Superpage FIFO
/// If it returns 0, use the default of MAX_SUPERPAGE_DESCRIPTORS_DEFAULT
static constexpr Register MAX_SUPERPAGE_DESCRIPTORS(0x00000c04);

///*** bar2 ***///

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Board serial number
/// Must be accessed on BAR 2
static constexpr Register SERIAL_NUMBER_CTRL(0x00030804);
static constexpr uint32_t SERIAL_NUMBER_TRG(0x2);
static constexpr Register SERIAL_NUMBER(0x00030818);

/// Register containing compilation info of the firmware
/// Can be used as a sort of version number
static constexpr Register FIRMWARE_COMPILE_INFO(0x280);

/// Register containing compilation info of the firmware
/// Can be used as a sort of version number
static constexpr Register FIRMWARE_FEATURES(0x41c);

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

/// Register containing the userlogic Git hash
/// Must be accessed on BAR 2
static constexpr Register USERLOGIC_GIT_HASH(0x00c00004);

/// Register containing the first part of the Arria 10 chip ID
/// Must be accessed on BAR 2
static constexpr Register FPGA_CHIP_HIGH(0x00010014);

/// Register containing the second part of the Arria 10 chip ID
/// Must be accessed on BAR 2
static constexpr Register FPGA_CHIP_LOW(0x00010018);

/// Registers for controlling the DDG
static constexpr Register DDG_CTRL0(0x00d00000);
static constexpr Register DDG_CTRL2(0x00d00004);

/// Register to control BSP
static constexpr Register BSP_USER_CONTROL(0x00000018);

/// Register to access I2C SFP information
static constexpr Register BSP_I2C_SFP_1(0x00030200);

/// Register to access I2C minipod information
static constexpr Register BSP_I2C_MINIPODS(0x00030300);

/// Register to access the EEPROM flash
static constexpr Register BSP_I2C_EEPROM(0x00030800);

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

/// Registers for getting TTC info
static constexpr Register TTC_ONU_STICKY(0x00200014);
static constexpr Register TTC_ONU_STICKY_MON(0x0020001c);
static constexpr Register TTC_PON_QUALITY(0x0010000C);

/// Registers used for TTC calibration
static constexpr Register PON_WRAPPER_PLL(0x00224000);
static constexpr Register PON_WRAPPER_TX(0x00226000);
static constexpr Register PON_WRAPPER_REG(0x00222000);

/// Registers for configuring PON TX
static constexpr Register CLOCK_ONU_FPLL(0x00248000);
static constexpr Register CLOCK_PLL_CONTROL_ONU(0x00240018);
static constexpr Register ONU_USER_LOGIC(0x0022a000);

/// Register for getting FEC status
static constexpr Register ONU_FEC_COUNTERS(0x0022200c);

/// Registers for setting the Ref Gen
static constexpr Register ONU_USER_REFGEN(0x0022c000);
static constexpr Register REFGEN0_OFFSET(0x00000000);
static constexpr Register REFGEN1_OFFSET(0x00000004);
static constexpr Register ONU_MGT_STICKYS(0x00222014);

/// Registers for getting LTU info
static constexpr Register LTU_HBTRIG_CNT(0x00200004);
static constexpr Register LTU_PHYSTRIG_CNT(0x00200008);
static constexpr Register LTU_TOFTRIG_CNT(0x00200018);
static constexpr Register LTU_CALTRIG_CNT(0x00200018);
static constexpr Register LTU_EOX_SOX_CNT(0x0020000c);

//** GBT **//
/// Wrapper 0's base address
static constexpr Register WRAPPER0(0x00400000);

/// Wrapper 1's base address
static constexpr Register WRAPPER1(0x00500000);

/// GBT Wrapper configuration registers
static constexpr Register GBT_WRAPPER_BANK_OFFSET(0x00020000);
static constexpr Register GBT_BANK_LINK_OFFSET(0x00002000);
static constexpr Register GBT_LINK_REGS_OFFSET(0x00000000);

// Register for getting the GBT link status (i.e. sticky bit)
static constexpr Register GBT_LINK_STATUS(0x00000000);

/// Register for selecting the GBT link source (i.e. Internal Data Generator)
static constexpr Register GBT_LINK_SOURCE_SELECT(0x00000030);

/// Register for clearing the GBT link error counters
static constexpr Register GBT_LINK_CLEAR_ERRORS(0x00000038);

/// Registers for getting the RX and TX link frequencies
static constexpr Register GBT_LINK_RX_CLOCK(0x00000008);
static constexpr Register GBT_LINK_TX_CLOCK(0x00000004);

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

/// GBT registers to collect loopback stats
static constexpr Register GBT_WRAPPER_TEST_CTRL(0x00000008);
static constexpr Register GBT_LINK_MASK_LOW(0x00000028);
static constexpr Register GBT_LINK_MASK_MED(0x00000024);
static constexpr Register GBT_LINK_MASK_HIGH(0x00000020);
static constexpr Register GBT_LINK_FEC_MONITORING(0x0000001c);

/// Registers to set TX and RX GBT modes
static constexpr Register GBT_LINK_TX_CONTROL_OFFSET(0x0000002c);
static constexpr Register GBT_LINK_RX_CONTROL_OFFSET(0x0000003c);
static constexpr Register GBT_LINK_RX_ERROR_COUNT(0x00000010);
/*static constexpr uint32_t GBT_MODE_GBT(0x0);
static constexpr uint32_t GBT_MODE_WB(0x1);*/

//** DATAPATH WRAPPER **//
/// Datapath Wrapper 0 base address
static constexpr Register DWRAPPER_BASE0(0x00600000);

/// Datapath Wrapper 1 base address
static constexpr Register DWRAPPER_BASE1(0x00700000);

/// DATAPATH Data Generator Control Register
static constexpr Register DWRAPPER_DATAGEN_CONTROL(0x00000004);

/// Datapath Wrapper offset for "Global Registers"
static constexpr Register DWRAPPER_GREGS(0x00000000);

/// Datapath Wrapper offset for "Enabled(?) Registers"
static constexpr Register DWRAPPER_ENREG(0x00000000);

/// Datapath Wrapper configuration registers
static constexpr Register DATAPATHLINK_OFFSET(0x00040000);
static constexpr Register DATALINK_OFFSET(0x00002000);
static constexpr Register DATALINK_CONTROL(0x00000000);
// [15 - 0] FEE ID
// [23 - 16] SYSTEM ID
static constexpr Register DATALINK_IDS(0x00000004);
static constexpr Register VIRTUAL_LINKS_IDS(0x00100014);
/*static constexpr uint32_t GBT_PACKET(0x1); 
static constexpr uint32_t GBT_STREAMING(0x0);*/

/// Register containing the number of dropped packets
static constexpr Register DWRAPPER_DROPPED_PACKETS(0x0000001c);

/// Register containing the number of total packets per second
static constexpr Register DWRAPPER_TOTAL_PACKETS_PER_SEC(0x0000002c);

/// Register to set the trigger window size in gbt words
static constexpr Register DWRAPPER_TRIGGER_SIZE(0x00000034);

static constexpr Register DATALINK_PACKETS_REJECTED(0x00000008);
static constexpr Register DATALINK_PACKETS_ACCEPTED(0x0000000c);
static constexpr Register DATALINK_PACKETS_FORCED(0x00000010);

/// Registers to set the Flow Control
static constexpr Register FLOW_CONTROL_OFFSET(0x000c0000);
static constexpr Register FLOW_CONTROL_REGISTER(0x00000000);

/// Register for the CTP Emulator
static constexpr Register CTP_EMU_RUNMODE(0x00280010);
static constexpr Register CTP_EMU_CTRL(0x00280000);
static constexpr Register CTP_EMU_BCMAX(0x00280004);
static constexpr Register CTP_EMU_HBMAX(0x00280008);
static constexpr Register CTP_EMU_PRESCALER(0x0028000c);
static constexpr Register CTP_EMU_PHYSDIV(0x00280014);
static constexpr Register CTP_EMU_CALDIV(0x00280020);
static constexpr Register CTP_EMU_HCDIV(0x00280018);
static constexpr Register CTP_EMU_FBCT(0x00280024);
static constexpr Register CTP_EMU_ORBIT_INIT(0x00280028);

/// Registers for the Pattern Player
static constexpr Register PATPLAYER_CFG(0x00260000);
static constexpr Register PATPLAYER_IDLE_PATTERN_0(0x00260004);
static constexpr Register PATPLAYER_IDLE_PATTERN_1(0x00260008);
static constexpr Register PATPLAYER_IDLE_PATTERN_2(0x0026000c);
static constexpr Register PATPLAYER_SYNC_PATTERN_0(0x00260010);
static constexpr Register PATPLAYER_SYNC_PATTERN_1(0x00260014);
static constexpr Register PATPLAYER_SYNC_PATTERN_2(0x00260018);
static constexpr Register PATPLAYER_RESET_PATTERN_0(0x0026001c);
static constexpr Register PATPLAYER_RESET_PATTERN_1(0x00260020);
static constexpr Register PATPLAYER_RESET_PATTERN_2(0x00260024);
static constexpr Register PATPLAYER_SYNC_CNT(0x00260028);
static constexpr Register PATPLAYER_DELAY_CNT(0x0026002c);
static constexpr Register PATPLAYER_RESET_CNT(0x00260030);
static constexpr Register PATPLAYER_TRIGGER_SEL(0x00260034);

//** I2C **//
/// I2C base addresses
static constexpr Register SI5345_1(0x00030500);
static constexpr Register SI5345_2(0x00030600);
static constexpr Register SI5344(0x00030400);

/// User Logic related addresses
static constexpr Register USER_LOGIC_RESET(0x00c80000);
static constexpr Register USER_LOGIC_EVSIZE(0x00c80004);
static constexpr Register USER_LOGIC_EVSIZE_RAND(0x00c80008);
static constexpr Register USER_LOGIC_SYSTEM_ID(0x00c8000c);
static constexpr Register USER_LOGIC_LINK_ID(0x00c80010);

/// Register to adjust the TimeFrame length (31 downto 20)
static constexpr Register TIME_FRAME_LENGTH(0x00000c00);

} // namespace Registers
} // namespace Cru
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_CONSTANTS_H_
