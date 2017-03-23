/************************************************
*  ddl_def.h                                    *
*  last update:        22/01/2007               *
*         common file for old and new ddl cards *
*  written by: Peter Csato and Ervin Denes      *
*  modified by: Pascal Boeschoten               *
************************************************/

#pragma once

namespace AliceO2 {
namespace Rorc {
/// DDL definitions
namespace Ddl {

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
constexpr int DEST_RORC = 0;
constexpr int DEST_DIU  = 1;
constexpr int DEST_SIU  = 2;
constexpr int DEST_DSI  = 3;
constexpr int DEST_FEE  = 4;

/// FEE commands
namespace Fee {
constexpr int RDYRX  = 1;  // Ready to Receive
constexpr int EOBTR  = 11; // End of Block Transfer
constexpr int STBWR  = 13; // Start of Block Write
constexpr int STBRD  = 5;  // Start of Block Read
constexpr int FECTRL = 12; // Front-end control
constexpr int FESTRD = 4;  // Front-end status readout
} // namespace Fee

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

namespace Diu {
// DIU version
constexpr int NOT_DEF  = -1;
constexpr int NO_DIU   = 0;
constexpr int OLD      = 1;
constexpr int NEW      = 2;
constexpr int EMBEDDED = 3;
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

namespace Siu {
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

/// status/error bits for OLD link cards
namespace Old {
namespace Diu {
// DIU status/error bits
constexpr int oDIU_LOOP  = 0x40000000; // DIU loop-back mode
constexpr int oLOSS_SIGN = 0x20000000; // loss of signal
constexpr int oD_RTOUT   = 0x10000000; // receiver synchronisation time-out
constexpr int oD_LOSY    = 0x08000000; // loss or word synchronisation
constexpr int oD_RDERR   = 0x04000000; // running disparity error
constexpr int oD_INVRX   = 0x02000000; // invalide receive word
constexpr int oD_CERR    = 0x01000000; // CRC error
constexpr int oD_UNREC   = 0x00800000; // unrecognised ordered set received
constexpr int oD_DOUT    = 0x00400000; // data word out of frame
constexpr int oD_IFDL    = 0x00200000; // illegal frame delimiter
constexpr int oD_LONG    = 0x00100000; // too long frame
constexpr int oD_RXOV    = 0x00080000; // received data/status overflow
constexpr int oD_LTOUT   = 0x00040000; // link initialisation time-out
// masks for DIU port states
constexpr int oDIU_NOSYNC = 0x00007000; // receiver not synchronised state
constexpr int oDIU_RSTSIU = 0x00006000; // DIU idle (reset SIU) state
constexpr int oDIU_FAIL   = 0x00005000; // DIU fail state
constexpr int oDIU_OFFL   = 0x00004000; // DIU offline state
constexpr int oDIU_LRES   = 0x00003000; // DIU LRES metastable state
constexpr int oDIU_START  = 0x00002000; // DIU START metastable state
constexpr int oDIU_ACCED  = 0x00001000; // DIU ACCED metastable state
constexpr int oLINK_ACT   = 0x00000000; // link active state
} // namespace Diu

namespace Siu {
// masks for received ordered sets
constexpr int oSIU_SRST  = 0x00038000; // Remote DIU in Reset state, sends SRST
constexpr int oSIU_FAIL  = 0x00030000; // Remote SIU/DIU in fail state, sends Not_Op
constexpr int oSIU_OFFL  = 0x00028000; // Remote SIU/DIU in offl. state, sends Oper
constexpr int oSIU_LINIT = 0x00020000; // Remote DIU is sending L_Init
constexpr int oSIU_ACT   = 0x00018000; // Remote SIU/DIU in active state, sends Idle
constexpr int oSIU_XOFF  = 0x00010000; // Remote SIU/DIU sends Xoff
constexpr int oSIU_XON   = 0x00008000; // Remote SIU/DIU sends Xon
constexpr int oSIU_ELSE  = 0x00000000; // Remote SIU/DIU is sending data or delimiter

// SIU status/error bits
constexpr int oS_LONGE   = 0x40000000; // too long event or read data block
constexpr int oS_IFEDS   = 0x20000000; // illegal FEE data/status
constexpr int oS_TXOF    = 0x10000000; // transmit FIFO overflow
constexpr int oS_IWDAT   = 0x08000000; // illegal write data word
constexpr int oS_WBLER   = 0x04000000; // write data block length error
constexpr int oS_RXOV    = 0x02000000; // receive FIFO overflow
constexpr int oS_LONGD   = 0x01000000; // too long data frame
constexpr int oS_LONGC   = 0x00800000; // too long command frame
constexpr int oS_OSIN    = 0x00400000; // ordered set inside a frame
constexpr int oS_DOUT    = 0x00200000; // data out of receive frame
constexpr int oS_LPERR   = 0x00100000; // link protocol error
constexpr int oS_CHERR   = 0x00080000; // check summ error in receive frame
constexpr int oS_UNREC   = 0x00040000; // unrecognised ordered set
constexpr int oS_INVRX   = 0x00020000; // invalid receive word
constexpr int oS_WALER   = 0x00010000; // word alignment error
constexpr int oS_ISPCH   = 0x00008000; // illegal special character
constexpr int oS_RDERR   = 0x00004000; // running disparity error
constexpr int oS_IRXCD   = 0x00002000; // illegal receive code
constexpr int oS_BUFER   = 0x00001000; // elastic buffer over/under run
} // namespace Siu
} // namespace Old

} // namespace Ddl
} // namespace Rorc
} // namespace AliceO2
