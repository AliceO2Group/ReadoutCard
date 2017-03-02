#pragma once

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

// flash interface registers
constexpr int F_IFDSR   = 60;     ///< Flash interface data and status reg
constexpr int F_IADR    = 61;     ///< Flash address register
constexpr int F_LRD     = 62;     ///< FLASH READY register
constexpr int F_RES0xFC = 63;     ///< reserved (0x0FC) register

// M-RORC function registers
constexpr int M_CSR     = 64;     ///< M-RORC control& status register
constexpr int M_IDR     = 65;     ///< M-RORC ID register
constexpr int M_RESx108 = 66;     ///< reserved (0x108) register
constexpr int M_RESx10C = 67;     ///< reserved (0x10C) register
constexpr int M_RXCSR   = 68;     ///< Receiver control and status register
constexpr int M_RXERR   = 69;     ///< Receiver error register
constexpr int M_RXDCR   = 70;     ///< Receiver data counter register
constexpr int M_RXDR    = 71;     ///< Receiver data register
constexpr int M_TXCSR   = 72;     ///< Transmitter control and status register
constexpr int M_TXDL    = 73;     ///< Transmitter data register, low
constexpr int M_TXDH    = 74;     ///< Transmitter data register, high
constexpr int M_TXDCR   = 75;     ///< Transmitter data counter register
constexpr int M_EIDFS   = 76;     ///< Event ID FIFO status register
constexpr int M_EIDFDL  = 77;     ///< Event ID FIFO data register, low
constexpr int M_EIDFDH  = 78;     ///< Event ID FIFO data register, high
constexpr int M_RESx13C = 79;     ///< reserved (0x13C) register

// FLASH bits (C-RORC FIDS and FIAD registers)
constexpr int FLASH_SN_ADDRESS = 0x01470000;
constexpr int RORC_SN_POSITION = 33;
constexpr int RORC_SN_LENGTH   = 5;

constexpr int DRORC_CMD_RESET_RORC       = 0x00000001;   //bit  0
constexpr int DRORC_CMD_RESET_CHAN       = 0x00000002;   //bit  1
constexpr int DRORC_CMD_CLEAR_RORC_ERROR = 0x00000008;   //bit  3

// CCSR commands
constexpr int DRORC_CMD_RESET_DIU      = 0x00000001;   //bit  0
constexpr int DRORC_CMD_CLEAR_FIFOS    = 0x00000002;   //bit  1
constexpr int DRORC_CMD_CLEAR_RXFF     = 0x00000004;   //bit  2
constexpr int DRORC_CMD_CLEAR_TXFF     = 0x00000008;   //bit  3
constexpr int DRORC_CMD_CLEAR_ERROR    = 0x00000010;   //bit  4
constexpr int DRORC_CMD_CLEAR_COUNTERS = 0x00000020;   //bit  5

constexpr int DRORC_CMD_DATA_TX_ON_OFF = 0x00000100;   //bit  8
constexpr int DRORC_CMD_DATA_RX_ON_OFF = 0x00000200;   //bit  9
constexpr int DRORC_CMD_START_DG       = 0x00000400;   //bit 10
constexpr int DRORC_CMD_STOP_DG        = 0x00000800;   //bit 11
constexpr int DRORC_CMD_LOOPB_ON_OFF   = 0x00001000;   //bit 12
//------------------- pRORC ----------------------------------

constexpr int PRORC_CMD_RESET_SIU     = 0x00F1;
constexpr int PRORC_PARAM_LOOPB       = 0x1;

constexpr int RORC_STATUS_OK                =    0;
constexpr int RORC_STATUS_ERROR             =   -1;
constexpr int RORC_INVALID_PARAM            =   -2;

constexpr int RORC_LINK_NOT_ON              =   -4;
constexpr int RORC_CMD_NOT_ALLOWED          =   -8;
constexpr int RORC_NOT_ACCEPTED             =  -16;
constexpr int RORC_NOT_ABLE                 =  -32;
constexpr int RORC_TIMEOUT                  =  -64;
constexpr int RORC_FF_FULL                  = -128;
constexpr int RORC_FF_EMPTY                 = -256;

// RORC initialization and reset options
constexpr int RORC_RESET_FF       =   1;   ///< reset Free FIFOs
constexpr int RORC_RESET_RORC     =   2;   ///< reset RORC
constexpr int RORC_RESET_DIU      =   4;   ///< reset DIU
constexpr int RORC_RESET_SIU      =   8;   ///< reset SIU
constexpr int RORC_LINK_UP        =  16;   ///< init link
constexpr int RORC_RESET_FEE      =  32;   ///< reset Front-End
constexpr int RORC_RESET_FIFOS    =  64;   ///< reset RORC's FIFOS (not Free FIFO)
constexpr int RORC_RESET_ERROR    = 128;   ///< reset RORC's error register
constexpr int RORC_RESET_COUNTERS = 256;   ///< reset RORC's event number counters

constexpr int RORC_RESET_ALL      = 0x000001FF;   //bits 8-0

constexpr int RORC_DG_INFINIT_EVENT = 0;

constexpr int DIU       =  1;
constexpr int RandCIFST =  0;     ///< Read & Clear Interface Status Word
constexpr int CTSTW     =  0;     ///< CTSTW
constexpr int CTSTW_TO  =  1;     ///< CTSTW for Front-End TimeOut
constexpr int ILCMD     =  2;     ///< CTSTW for illegal command
constexpr int IFSTW     = 12;     ///< IFSTW

constexpr int DRORC_STAT_LINK_DOWN         = 0x00002000;   ///< bit 13
constexpr int DRORC_STAT_CMD_NOT_EMPTY     = 0x00010000;   ///< bit 16
constexpr int DRORC_STAT_RXAFF_EMPTY       = 0x00040000;   ///< bit 18
constexpr int DRORC_STAT_RXAFF_FULL        = 0x00080000;   ///< bit 19
constexpr int DRORC_STAT_RXSTAT_NOT_EMPTY  = 0x00800000;   ///< bit 23
constexpr int DRORC_STAT_RXDAT_ALMOST_FULL = 0x01000000;   ///< bit 24
constexpr int DRORC_STAT_RXDAT_NOT_EMPTY   = 0x02000000;   ///< bit 25

constexpr int RORC_DATA_BLOCK_NOT_ARRIVED      = 0;
constexpr int RORC_NOT_END_OF_EVENT_ARRIVED    = 1;
constexpr int RORC_LAST_BLOCK_OF_EVENT_ARRIVED = 2;

constexpr int RORC_REVISION_PRORC  = 1;
constexpr int RORC_REVISION_DRORC  = 2;
constexpr int RORC_REVISION_INTEG  = 3;
constexpr int RORC_REVISION_DRORC2 = 4;
constexpr int RORC_REVISION_PCIEXP = 5;
constexpr int RORC_REVISION_CHAN4  = 6;
constexpr int RORC_REVISION_CRORC  = 7;
