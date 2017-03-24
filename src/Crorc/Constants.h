/// \file Constants.h
/// \brief Definitions of RORC related constants
/// Based on ddl_def.h and rorc.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

namespace AliceO2
{
namespace Rorc
{

namespace Rorc
{
constexpr int RCSR =      0;     ///< RORC Control and Status register
constexpr int RERR =      1;     ///< RORC Error register
constexpr int RFID =      2;     ///< Firmware ID
constexpr int RHID =      3;     ///< Hardware ID
constexpr int C_CSR =     4;     ///< channel Control and Status register
constexpr int C_ERR =     5;     ///< channel Error register
constexpr int C_DCR =     6;     ///< channel DDL Command register
constexpr int C_DSR =     7;     ///< channel DDL Status register
constexpr int C_DG1 =     8;     ///< channel Data Generator param 1
constexpr int C_DG2 =     9;     ///< channel Data Generator param 2
constexpr int C_DG3 =    10;     ///< channel Data Generator param 3
constexpr int C_DG4 =    11;     ///< channel Data Generator param 4
constexpr int C_DGS =    12;     ///< channel Data Generator Status
constexpr int C_RRBAR =  13;     ///< channel Receive Report Base Address
constexpr int C_RAFL =   14;     ///< channel Receive Address FIFO Low
constexpr int C_RAFH =   15;     ///< channel Receive Address FIFO High
constexpr int C_TRBAR =  16;     ///< channel Transmit Report Base Address
constexpr int C_TAFL =   17;     ///< channel Transmit Address FIFO Low
constexpr int C_TAFH =   18;     ///< channel Transmit Address FIFO High
constexpr int C_TMCS =   19;     ///< channel Traffic Monitoring Control/Status
constexpr int C_DDLFE =  20;     ///< channel DDL FIFO Empty counter
constexpr int C_DDLFF =  21;     ///< channel DDL FIFO Full  counter
constexpr int C_PCIFE =  22;     ///< channel PCI FIFO Empty counter
constexpr int C_PCIFF =  23;     ///< channel PCI FIFO Full  counter
constexpr int C_DDLFR =  24;     ///< channel DDL FIFO Read  counter
constexpr int C_DDLFW =  25;     ///< channel DDL FIFO Write counter
constexpr int C_PCIFR =  26;     ///< channel PCI FIFO Read  counter
constexpr int C_PCIFW =  27;     ///< channel PCI FIFO Write counter
constexpr int C_RXDC =   28;     ///< channel Receive  DMA count register
constexpr int C_TXDC =   29;     ///< channel Transmit DMA count register
constexpr int C_RDAL =   30;     ///< channel Receive  DMA address register Low
constexpr int C_TDAL =   31;     ///< channel Transmit DMA address register Low

// high addresses for 64 bit
constexpr int C_RAFX =   32;     ///< Receive Address FIFO Extension register
constexpr int C_RRBX =   33;     ///< Rx Report Base Address Extension register
constexpr int C_TAFX =   34;     ///< Transmit Address FIFO Extension register
constexpr int C_TRBX =   35;     ///< Tx Report Base Address Extension register
constexpr int C_RDAH =   36;     ///< channel Receive  DMA address register High
constexpr int C_TDAH =   37;     ///< channel Transmit DMA address register High

// occupancy registers
constexpr int C_RAFO =   38;     ///< Rx Address FIFO Occupancy register
constexpr int C_TAFO =   39;     ///< Tx Address FIFO Occupancy register

// new registers for CRORC
constexpr int C_LKST =   40;     ///< link status register
constexpr int C_HOFF =   41;     ///< HLT OFF counter register

// reserved register addresses
namespace Reserved
{
constexpr int C_RES0xA8 = 42;     ///< reserved (0x0A8) register
constexpr int C_RES0xAC = 43;     ///< reserved (0x0AC) register
constexpr int C_RES0xB0 = 44;     ///< reserved (0x0B0) register
constexpr int C_RES0xB4 = 45;     ///< reserved (0x0B4) register
constexpr int C_RES0xB8 = 46;     ///< reserved (0x0B8) register
constexpr int C_RES0xBC = 47;     ///< reserved (0x0BC) register
constexpr int C_RES0xC0 = 48;     ///< reserved (0x0C0) register
constexpr int C_RES0xC4 = 49;     ///< reserved (0x0C4) register
constexpr int C_RES0xC8 = 50;     ///< reserved (0x0C8) register
constexpr int C_RES0xCC = 51;     ///< reserved (0x0CC) register
constexpr int C_RES0xD0 = 52;     ///< reserved (0x0D0) register
constexpr int C_RES0xD4 = 53;     ///< reserved (0x0D4) register
constexpr int C_RES0xD8 = 54;     ///< reserved (0x0D8) register
constexpr int C_RES0xDC = 55;     ///< reserved (0x0DC) register
constexpr int C_RES0xE0 = 56;     ///< reserved (0x0E0) register
constexpr int C_RES0xE4 = 57;     ///< reserved (0x0E4) register
constexpr int C_RES0xE8 = 58;     ///< reserved (0x0E8) register
constexpr int C_RES0xEC = 59;     ///< reserved (0x0EC) register
}

/// Flash interface registers
namespace Flash
{
constexpr int F_IFDSR   = 60;     ///< Flash interface data and status reg
constexpr int F_IADR    = 61;     ///< Flash address register
constexpr int F_LRD     = 62;     ///< FLASH READY register
constexpr int F_RES0xFC = 63;     ///< reserved (0x0FC) register
}

/// Serial number related constants
namespace Serial
{
constexpr int FLASH_ADDRESS = 0x01470000;
constexpr int POSITION = 33;
constexpr int LENGTH   = 5;
}

/// RCSR commands
namespace RcsrCommand
{
constexpr int RESET_RORC       = 0x00000001;   //bit  0
constexpr int RESET_CHAN       = 0x00000002;   //bit  1
constexpr int CLEAR_RORC_ERROR = 0x00000008;   //bit  3
}

/// C_CSR commands
namespace CcsrCommand
{
constexpr int RESET_DIU      = 0x00000001;   //bit  0
constexpr int CLEAR_FIFOS    = 0x00000002;   //bit  1
constexpr int CLEAR_RXFF     = 0x00000004;   //bit  2
constexpr int CLEAR_TXFF     = 0x00000008;   //bit  3
constexpr int CLEAR_ERROR    = 0x00000010;   //bit  4
constexpr int CLEAR_COUNTERS = 0x00000020;   //bit  5
constexpr int DATA_TX_ON_OFF = 0x00000100;   //bit  8
constexpr int DATA_RX_ON_OFF = 0x00000200;   //bit  9
constexpr int START_DG       = 0x00000400;   //bit 10
constexpr int STOP_DG        = 0x00000800;   //bit 11
constexpr int LOOPB_ON_OFF   = 0x00001000;   //bit 12
}

/// C_CSR status bits
namespace CcsrStatus
{
constexpr int LINK_DOWN         = 0x00002000;   ///< bit 13
constexpr int CMD_NOT_EMPTY     = 0x00010000;   ///< bit 16
constexpr int RXAFF_EMPTY       = 0x00040000;   ///< bit 18
constexpr int RXAFF_FULL        = 0x00080000;   ///< bit 19
constexpr int RXSTAT_NOT_EMPTY  = 0x00800000;   ///< bit 23
constexpr int RXDAT_ALMOST_FULL = 0x01000000;   ///< bit 24
constexpr int RXDAT_NOT_EMPTY   = 0x02000000;   ///< bit 25
}

/// DCR commands
namespace DcrCommand
{
constexpr int RESET_SIU = 0x00F1;
}

constexpr int PRORC_PARAM_LOOPB       = 0x1;

namespace Reset
{
// RORC initialization and reset options
constexpr int FF       =   1;   ///< reset Free FIFOs
constexpr int RORC     =   2;   ///< reset RORC
constexpr int DIU      =   4;   ///< reset DIU
constexpr int SIU      =   8;   ///< reset SIU
constexpr int LINK_UP  =  16;   ///< init link
constexpr int FEE      =  32;   ///< reset Front-End
constexpr int FIFOS    =  64;   ///< reset RORC's FIFOS (not Free FIFO)
constexpr int ERROR    = 128;   ///< reset RORC's error register
constexpr int COUNTERS = 256;   ///< reset RORC's event number counters
constexpr int ALL      = 0x000001FF; ///< bits 8-0
} // namespace Reset

constexpr int RORC_DG_INFINIT_EVENT = 0;

constexpr int DIU       =  1;
constexpr int RandCIFST =  0;     ///< Read & Clear Interface Status Word
constexpr int CTSTW     =  0;     ///< CTSTW
constexpr int CTSTW_TO  =  1;     ///< CTSTW for Front-End TimeOut
constexpr int ILCMD     =  2;     ///< CTSTW for illegal command
constexpr int IFSTW     = 12;     ///< IFSTW

constexpr int RORC_DATA_BLOCK_NOT_ARRIVED      = 0;
constexpr int RORC_NOT_END_OF_EVENT_ARRIVED    = 1;
constexpr int RORC_LAST_BLOCK_OF_EVENT_ARRIVED = 2;
} // namespace Rorc

/// DDL definitions
namespace Ddl
{
// wait cycles for DDL respond
constexpr int TIMEOUT        = 100000;
constexpr int RESPONSE_TIME  = 1000;
constexpr unsigned long long MAX_WAIT_CYCLE = 0x7fffffffffffffffULL;
constexpr int MAX_REPLY      = 4;
constexpr int MAX_HW_ID      = 64;

// maximum size of DDL events
constexpr int MAX_WORD         = 0x07FFFF;
constexpr int MAX_BYTE         = 0x1FFFFC;
constexpr int HEADER_SIZE_BYTE = 32;
constexpr int HEADER_SIZE_WORD = 8;

// maximum size of a simulated event sent by the DDG
constexpr int MAX_TX_WORD = 0xFFFFFF;

//  destination field
namespace Destination
{
constexpr int RORC = 0;
constexpr int DIU  = 1;
constexpr int SIU  = 2;
constexpr int DSI  = 3;
constexpr int FEE  = 4;
} // namespace Destination

// interface commands
constexpr int LRST      = 10; // Link Reset
constexpr int SUSPEND   = 10; // Go to Power Off state
constexpr int LINIT     = 11; // Link Initialisation
constexpr int WAKEUP    = 11; // Wake up from Power Off state
constexpr int SRST      = 15; // SIU Reset
constexpr int IFLOOP    = 9;  // DIU Transmitter or SIU Receiver Loop-back
constexpr int TSTOP     = 12; // Self-test Stop
constexpr int TSTMODE   = 13; // Self-test Start
constexpr int DTCC      = 8;  // Data Transmission Check Command
constexpr int RFWVER    = 4;  // Read Firmware Version of DIU or SIU
constexpr int RHWVER    = 6;  // Read Hardware Version of DIU or SIU
constexpr int RPMVAL    = 7;  // Read Power Monitor Value
constexpr int RandCIFST = 0;  // Read & Clear Interface Status Word

// PRBS Test Mode command parameters
constexpr int STRPRBS = 1; // Start PRBS test
constexpr int STPPRBS = 0; // Stop PRBS test
constexpr int CLRPRBS = 3; // Clear PRBS Counter
constexpr int RDPRBS  = 0x40000 ; // Read PRBS Counter

// status codes
constexpr int CTSTW    =  0; // CTSTW
constexpr int CTSTW_TO =  1; // CTSTW for Front-End TimeOut
constexpr int ILCMD    =  2; // CTSTW for illegal command
constexpr int FESTW    =  4; // FESTW
constexpr int HWSTW    =  6; // HWSTW
constexpr int PMVAL    =  7; // Power Monitor Value
constexpr int DTSTW    =  8; // DTSTW
constexpr int DTSTW_TO =  9; // DTSTW with Front-End TimeOut
constexpr int IFSTW    = 12; // IFSTW
constexpr int TEVAL    = 13; // Test Mode Error Value
constexpr int FWSTW    = 14; // FWSTW

// data transmission status word
constexpr int DTSW             = 0x00000082;
constexpr int CONTINUATION_BIT = 0x00000100;
constexpr int EOB              = 0x000000b4; // End of block command
constexpr int CTSW             = 0x00000002; // Command Transmission Status word

// masks for DIU/SIU status word
constexpr int STMASK    = 0xFFFFF0FF; // status word without ID#
constexpr int DIUMASK   = 0xBFFC7000; // DIU port states (without loop-back bit)
constexpr int REMMASK   = 0x00038000; // Remote SIU/DIU states
constexpr int DIUSTMASK = 0x00007000; // DIU port states
constexpr int SIUSTMASK = 0x00007000; // SIU port states
constexpr int DIUERMASK = 0xBFFC0000; // DIU error states
} // namespace Ddl

/// FEE commands
namespace Fee
{
constexpr int RDYRX  = 1;  // Ready to Receive
constexpr int EOBTR  = 11; // End of Block Transfer
constexpr int STBWR  = 13; // Start of Block Write
constexpr int STBRD  = 5;  // Start of Block Read
constexpr int FECTRL = 12; // Front-end control
constexpr int FESTRD = 4;  // Front-end status readout
} // namespace Fee

namespace Diu
{
// status/error bits for NEW (CMC connector) link cards
// DIU status/error bits
constexpr int ERROR_BIT = 0x80000000; // error bit
constexpr int DIU_LOOP  = 0x40000000; // DIU loop-back mode
constexpr int LOSS_SYNC = 0x20000000; // loss of synchronization
constexpr int D_TXOF    = 0x10000000; // transmit data/status overflow
constexpr int D_RES1    = 0x08000000; // reserved
constexpr int D_OSINFR  = 0x04000000; // ordered set in frame
constexpr int D_INVRX   = 0x02000000; // invalide receive word
constexpr int D_CERR    = 0x01000000; // CRC error
constexpr int D_RES2    = 0x00800000; // reserved
constexpr int D_DOUT    = 0x00400000; // data word out of frame
constexpr int D_IFDL    = 0x00200000; // illegal frame delimiter
constexpr int D_LONG    = 0x00100000; // too long frame
constexpr int D_RXOF    = 0x00080000; // received data/status overflow
constexpr int D_FRERR   = 0x00040000; // error in receive frame
// masks for DIU port states
constexpr int DIU_TSTM  = 0x00007000; // DIU in PRBS Test Mode state
constexpr int DIU_POFF  = 0x00006000; // DIU in Power Off state
constexpr int DIU_LOS   = 0x00005000; // DIU in Offline and Loss of Synchro state
constexpr int DIU_NOSIG = 0x00004000; // DIU in Offline and No Signal state
constexpr int DIU_WAIT  = 0x00003000; // DIU in Waiting for Power Off state
constexpr int DIU_ONL   = 0x00002000; // DIU in Online state
constexpr int DIU_OFFL  = 0x00001000; // DIU in Offline state
constexpr int DIU_POR   = 0x00000000; // DIU in Power On Reset state
} // namespace Diu

namespace Siu
{
// SIU status/error bits
constexpr int S_LONGE   = 0x40000000; // too long event or read data block
constexpr int S_IFEDS   = 0x20000000; // illegal FEE data/status
constexpr int S_TXOF    = 0x10000000; // transmit FIFO overflow
constexpr int S_IWDAT   = 0x08000000; // illegal write data word
constexpr int S_OSINFR  = 0x04000000; // ordered set inside a frame
constexpr int S_INVRX   = 0x02000000; // invalid character inside a frame
constexpr int S_CERR    = 0x01000000; // CRC error
constexpr int S_DJLERR  = 0x00800000; // DTCC / JTCC error
constexpr int S_DOUT    = 0x00400000; // data out of receive frame
constexpr int S_IFDL    = 0x00200000; // illegal frame delimiter (SOF)
constexpr int S_LONG    = 0x00100000; // too long receive frame
constexpr int S_RXOF    = 0x00080000; // receive FIFO overflow
constexpr int S_FRERR   = 0x00040000; // error in receive frame
constexpr int S_LPERR   = 0x00020000; // link protocol error
constexpr int S_LBMOD   = 0x00010000; // SIU in Loop Back Mode
constexpr int S_OPTRAN  = 0x00008000; // open FEE transaction

// masks for SIU port states
constexpr int SIU_RESERV = 0x00007000; // reserved
constexpr int SIU_POFF   = 0x00006000; // SIU in Power Off state
constexpr int SIU_LOS    = 0x00005000; // SIU in Offline and Loss of Synchro state
constexpr int SIU_NOSIG  = 0x00004000; // SIU in Offline and No Signal state
constexpr int SIU_WAIT   = 0x00003000; // SIU in Waiting for Power Off state
constexpr int SIU_ONL    = 0x00002000; // SIU in Online state
constexpr int SIU_OFFL   = 0x00001000; // SIU in Offline state
constexpr int SIU_POR    = 0x00000000; // SIU in Power On Reset state
} // namespace Siu

} // namespace Rorc
} // namespace AliceO2
