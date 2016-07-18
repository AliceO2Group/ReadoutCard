#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "stdint.h"
#include "linux/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "stword.h"
#include "ddl.h"

#define RCSR       0     /* RORC Control and Status register          */
#define RERR       1     /* RORC Error register                       */
#define RFID       2     /* Firmware ID                               */
#define RHID       3     /* Hardware ID                               */
#define C_CSR      4     /* channel Control and Status register       */
#define C_ERR      5     /* channel Error register                    */
#define C_DCR      6     /* channel DDL Command register              */
#define C_DSR      7     /* channel DDL Status register               */
#define C_DG1      8     /* channel Data Generator param 1            */
#define C_DG2      9     /* channel Data Generator param 2            */
#define C_DG3     10     /* channel Data Generator param 3            */
#define C_DG4     11     /* channel Data Generator param 4            */
#define C_DGS     12     /* channel Data Generator Status             */
#define C_RRBAR   13     /* channel Receive Report Base Address       */
#define C_RAFL    14     /* channel Receive Address FIFO Low          */
#define C_RAFH    15     /* channel Receive Address FIFO High         */
#define C_TRBAR   16     /* channel Transmit Report Base Address      */
#define C_TAFL    17     /* channel Transmit Address FIFO Low         */
#define C_TAFH    18     /* channel Transmit Address FIFO High        */
#define C_TMCS    19     /* channel Traffic Monitoring Control/Status */
#define C_DDLFE   20     /* channel DDL FIFO Empty counter            */
#define C_DDLFF   21     /* channel DDL FIFO Full  counter            */
#define C_PCIFE   22     /* channel PCI FIFO Empty counter            */
#define C_PCIFF   23     /* channel PCI FIFO Full  counter            */
#define C_DDLFR   24     /* channel DDL FIFO Read  counter            */
#define C_DDLFW   25     /* channel DDL FIFO Write counter            */
#define C_PCIFR   26     /* channel PCI FIFO Read  counter            */
#define C_PCIFW   27     /* channel PCI FIFO Write counter            */
#define C_RXDC    28     /* channel Receive  DMA count register       */
#define C_TXDC    29     /* channel Transmit DMA count register       */
#define C_RDAL    30     /* channel Receive  DMA address register Low */
#define C_TDAL    31     /* channel Transmit DMA address register Low */

/* high addresses for 64 bit */
#define C_RAFX    32     /* Receive Address FIFO Extension register   */
#define C_RRBX    33     /* Rx Report Base Address Extension register */
#define C_TAFX    34     /* Transmit Address FIFO Extension register  */
#define C_TRBX    35     /* Tx Report Base Address Extension register */
#define C_RDAH    36     /* channel Receive  DMA address register High*/
#define C_TDAH    37     /* channel Transmit DMA address register High*/

/* occupancy registers */
#define C_RAFO    38     /* Rx Address FIFO Occupancy register        */
#define C_TAFO    39     /* Tx Address FIFO Occupancy register        */

/* new registers for CRORC */
#define C_LKST    40     /* link status register                      */
#define C_HOFF    41     /* HLT OFF counter register                  */

/* reserved register addresses */
#define C_RES0xA8 42     /* reserved (0x0A8) register                 */
#define C_RES0xAC 43     /* reserved (0x0AC) register                 */
#define C_RES0xB0 44     /* reserved (0x0B0) register                 */
#define C_RES0xB4 45     /* reserved (0x0B4) register                 */
#define C_RES0xB8 46     /* reserved (0x0B8) register                 */
#define C_RES0xBC 47     /* reserved (0x0BC) register                 */
#define C_RES0xC0 48     /* reserved (0x0C0) register                 */
#define C_RES0xC4 49     /* reserved (0x0C4) register                 */
#define C_RES0xC8 50     /* reserved (0x0C8) register                 */
#define C_RES0xCC 51     /* reserved (0x0CC) register                 */
#define C_RES0xD0 52     /* reserved (0x0D0) register                 */
#define C_RES0xD4 53     /* reserved (0x0D4) register                 */
#define C_RES0xD8 54     /* reserved (0x0D8) register                 */
#define C_RES0xDC 55     /* reserved (0x0DC) register                 */
#define C_RES0xE0 56     /* reserved (0x0E0) register                 */
#define C_RES0xE4 57     /* reserved (0x0E4) register                 */
#define C_RES0xE8 58     /* reserved (0x0E8) register                 */
#define C_RES0xEC 59     /* reserved (0x0EC) register                 */

/* flash interface registers */
#define F_IFDSR   60     /* Flash interface data and status reg       */
#define F_IADR    61     /* Flash address register                    */
#define F_LRD     62     /* FLASH READY register                      */
#define F_RES0xFC 63     /* reserved (0x0FC) register                 */

/* M-RORC function registers */
#define M_CSR     64     /* M-RORC control& status register           */
#define M_IDR     65     /* M-RORC ID register                        */
#define M_RESx108 66     /* reserved (0x108) register                 */
#define M_RESx10C 67     /* reserved (0x10C) register                 */
#define M_RXCSR   68     /* Receiver control and status register      */
#define M_RXERR   69     /* Receiver error register                   */
#define M_RXDCR   70     /* Receiver data counter register            */
#define M_RXDR    71     /* Receiver data register                    */
#define M_TXCSR   72     /* Transmitter control and status register   */
#define M_TXDL    73     /* Transmitter data register, low            */
#define M_TXDH    74     /* Transmitter data register, high           */
#define M_TXDCR   75     /* Transmitter data counter register         */
#define M_EIDFS   76     /* Event ID FIFO status register             */
#define M_EIDFDL  77     /* Event ID FIFO data register, low          */
#define M_EIDFDH  78     /* Event ID FIFO data register, high         */
#define M_RESx13C 79     /* reserved (0x13C) register                 */

/*
 * FLASH bits (C-RORC FIDS and FIAD registers)
 */
#define FLASH_SN_ADDRESS 0x01470000
#define RORC_SN_POSITION 33
#define RORC_SN_LENGTH 5

#define DRORC_CMD_RESET_RORC       0x00000001   //bit  0
#define DRORC_CMD_RESET_CHAN       0x00000002   //bit  1
#define DRORC_CMD_CLEAR_RORC_ERROR 0x00000008   //bit  3

/* CCSR commands */

#define DRORC_CMD_RESET_DIU      0x00000001   //bit  0
#define DRORC_CMD_CLEAR_FIFOS    0x00000002   //bit  1
#define DRORC_CMD_CLEAR_RXFF     0x00000004   //bit  2
#define DRORC_CMD_CLEAR_TXFF     0x00000008   //bit  3
#define DRORC_CMD_CLEAR_ERROR    0x00000010   //bit  4
#define DRORC_CMD_CLEAR_COUNTERS 0x00000020   //bit  5

#define DRORC_CMD_DATA_TX_ON_OFF 0x00000100   //bit  8
#define DRORC_CMD_DATA_RX_ON_OFF 0x00000200   //bit  9
#define DRORC_CMD_START_DG       0x00000400   //bit 10
#define DRORC_CMD_STOP_DG        0x00000800   //bit 11
#define DRORC_CMD_LOOPB_ON_OFF   0x00001000   //bit 12
/*------------------- pRORC ----------------------------------*/

#define PRORC_CMD_RESET_SIU      0x00F1
#define PRORC_PARAM_LOOPB        0x1

#define RORC_STATUS_OK                    0
#define RORC_STATUS_ERROR                -1
#define RORC_INVALID_PARAM               -2

#define RORC_LINK_NOT_ON                 -4
#define RORC_CMD_NOT_ALLOWED             -8
#define RORC_NOT_ACCEPTED               -16
#define RORC_NOT_ABLE                   -32
#define RORC_TIMEOUT                    -64
#define RORC_FF_FULL                   -128
#define RORC_FF_EMPTY                  -256
/*
 * RORC initialization and reset options
 */

#define RORC_RESET_FF         1   /* reset Free FIFOs */
#define RORC_RESET_RORC       2   /* reset RORC */
#define RORC_RESET_DIU        4   /* reset DIU */
#define RORC_RESET_SIU        8   /* reset SIU */
#define RORC_LINK_UP         16   /* init link */
#define RORC_RESET_FEE       32   /* reset Front-End */
#define RORC_RESET_FIFOS     64   /* reset RORC's FIFOS (not Free FIFO) */
#define RORC_RESET_ERROR    128   /* reset RORC's error register */
#define RORC_RESET_COUNTERS 256   /* reset RORC's event number counters */

#define RORC_RESET_ALL      0x000001FF   //bits 8-0

#define RORC_DG_INFINIT_EVENT  0

//#define DIU 1
//#define RandCIFST     0       /* Read & Clear Interface Status Word */
//#define CTSTW     0     /* CTSTW */
//#define CTSTW_TO  1     /* CTSTW for Front-End TimeOut */
//#define ILCMD     2     /* CTSTW for illegal command */
//#define IFSTW    12     /* IFSTW */

#define DRORC_STAT_LINK_DOWN          0x00002000   //bit 13
#define DRORC_STAT_CMD_NOT_EMPTY      0x00010000   //bit 16
#define DRORC_STAT_RXAFF_EMPTY        0x00040000   //bit 18
#define DRORC_STAT_RXAFF_FULL         0x00080000   //bit 19
#define DRORC_STAT_RXSTAT_NOT_EMPTY   0x00800000   //bit 23
#define DRORC_STAT_RXDAT_ALMOST_FULL  0x01000000   //bit 24
#define DRORC_STAT_RXDAT_NOT_EMPTY    0x02000000   //bit 25

#define RORC_DATA_BLOCK_NOT_ARRIVED       0
#define RORC_NOT_END_OF_EVENT_ARRIVED     1
#define RORC_LAST_BLOCK_OF_EVENT_ARRIVED  2

#define RORC_REVISION_PRORC  1
#define RORC_REVISION_DRORC  2
#define RORC_REVISION_INTEG  3
#define RORC_REVISION_DRORC2 4
#define RORC_REVISION_PCIEXP 5
#define RORC_REVISION_CHAN4  6
#define RORC_REVISION_CRORC  7

#define rorcReadReg(buff, reg_number) (*((volatile uint32_t*)buff + reg_number))
#define rorcWriteReg(buff, reg_number, reg_value) *((volatile uint32_t*)buff + reg_number) = reg_value

#define rorcCheckLink(buff) \
  ((rorcReadReg(buff, C_CSR) & DRORC_STAT_LINK_DOWN) ? RORC_LINK_NOT_ON \
                                                       : RORC_STATUS_OK)
#define rorcCheckCommandRegister(buff) \
  (rorcReadReg(buff, C_CSR) & DRORC_STAT_CMD_NOT_EMPTY)

#define rorcPutCommandRegister(buff, com) \
  (rorcWriteReg((buff), C_DCR, (com)))

#define rorcCheckRxStatus(buff) (rorcReadReg(buff, C_CSR) & \
				 DRORC_STAT_RXSTAT_NOT_EMPTY)

#define rorcCheckRxData(buff) (rorcReadReg(buff, C_CSR) & \
			       DRORC_STAT_RXDAT_NOT_EMPTY)
#define arch64 (sizeof(void *) > 4)

#define rorcPushRxFreeFifo(buff, blockAddress, blockLength, readyFifoIndex) \
  rorcWriteReg (buff, C_RAFX, (arch64 ? ((blockAddress) >> 32) : 0x0)); \
  rorcWriteReg (buff, C_RAFH, ((blockAddress) & 0xffffffff));		\
  rorcWriteReg (buff, C_RAFL, (((blockLength) << 8) | (readyFifoIndex)))

#define rorcHasData(fifo, index) \
  ((*((__u32*)fifo + 2*index + 1) == -1) ? RORC_DATA_BLOCK_NOT_ARRIVED : \
   ((*((__u32*)fifo + 2*index + 1) == 0)  ? RORC_NOT_END_OF_EVENT_ARRIVED : \
    RORC_LAST_BLOCK_OF_EVENT_ARRIVED)) 
#define rorcCheckLoopBack(buff) (rorcReadReg(buff, C_CSR) & \
				 DRORC_CMD_LOOPB_ON_OFF)
#define rorcChangeLoopBack(buff) rorcWriteReg(buff, C_CSR, \
					      DRORC_CMD_LOOPB_ON_OFF)

#define rorcPushTxFreeFifo(buff, blockAddress, blockLength, readyFifoIndex) \
  rorcWriteReg (buff, C_TAFX, (arch64 ? ((blockAddress)) >> 32 : 0x0)); \
  rorcWriteReg (buff, C_TAFH, ((blockAddress) & 0xffffffff));		\
  rorcWriteReg (buff, C_TAFL, (((blockLength) << 8) | (readyFifoIndex)))

/** RORC functions, for definitions see rorc.c */
void rorcReset (volatile void *buff, int option, int pci_loop_per_usec);
int rorcEmptyDataFifos(volatile void *buff, int empty_time);
int rorcArmDDL(volatile void *buff, int option, int diu_version, int pci_loop_per_usec);
int rorcCheckRxFreeFifo(volatile void *buff);
int rorcStartDataReceiver(volatile void *buff,
                          unsigned long readyFifoBaseAddress,
                          int rorc_revision);
int rorcStopDataReceiver(volatile void *buff);
int rorcStartTrigger(volatile void *buff, long long int timeout, stword_t *stw);
int rorcStopTrigger(volatile void *buff, long long int timeout, stword_t *stw);

__u32 rorcReadFw(volatile void *buff);
void rorcInterpretVersion(__u32 x);
int rorcArmDataGenerator(volatile void *buff, __u32 initEventNumber, __u32 initDataWord,
			 int dataPattern, int eventLen, int seed, int *rounded_len);
int rorcStartDataGenerator(volatile void *buff, __u32 maxLoop);
int rorcParamOn(volatile void *buff, int param);
int rorcParamOff(volatile void *buff);

void setLoopPerSec(long long int *loop_per_usec, double *pci_loop_per_usec, volatile void *buff);
unsigned initFlash(volatile void *buff, unsigned address, int sleept);
unsigned readFlashStatus(volatile void *buff, int sleept);
int checkFlashStatus(volatile void *buff, int timeout);
int unlockFlashBlock(volatile void *buff, unsigned address, int sleept);
int eraseFlashBlock(volatile void *buff, unsigned address, int sleept);
int writeFlashWord(volatile void *buff, unsigned address, int value, int sleept);
int readFlashWord(volatile void *buff, unsigned address, char *data, int sleept);

#ifdef __cplusplus
} /* extern "C" */
#endif

