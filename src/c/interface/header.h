/// \file header.h
/// \brief Header of the API for PDA based DMA for the CRORC and CRU.
///
///  Last updated: 26/04/2016
///
/// \author Tuan Mate Nguyen <tuan.mate.nguyen@cern.ch>

#ifndef PDA_DMA_INTERFACE_HEADER_H_
#define PDA_DMA_INTERFACE_HEADER_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <pda.h>

#include "../rorc/rorc.h"            // Contains the old rorc library functions that have been
                                     // modified in order to work with the PDA.

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_STARTS_ON_NEW_NODE (-1) // Macro for indicating if there is not enough space 
                                     // on the first SG node for one page.

//Global variables for configuration parameters.
uint32_t
  DMA_PAGE_LENGTH,// Page length.
  FIFO_ENTRIES,   // Number of software FIFO entries.
  DATA_LENGTH,    // Length of data written to each page.
  DATA_GENERATOR, // If data generator is used, LOOPBACK parameter is needed in this case.
  LOOPBACK,       // Gives the type of loopback (0: no loopback, 1: in DIU, 2: in SIU, 3: inside RORC)
  SW_FIFO_OFFSET, // Offset of the software FIFO from the starting address of the buffer.
  DATA_OFFSET,    // Offset of the data from the end of the software FIFO.
  FULL_OFFSET,    // Offset of the data from the from the starting address of the buffer.
  PATTERN,        // Data pattern parameter for the data generator
  INIT_VALUE,     // Initial value of the first data in a data block.
  RANDOM_SEED,    // Random seed parameter in case the data generator produces random data.
  MAX_EVENT,      // Maximum number of events.
  RESET_LEVEL,    // Reset level parameter (0: reset nothing, 1: reset RORC, 2: reset RORC
                  // and DIU, 3: reset RORC, DIU and SIU
  DDL_HEADER,     // Defines that the received fragment contains the Common Data Header.
  NO_RDYRX,       // Prevents sending the RDYRX and EOBTR commands. This switch is implicitly set when
                  // data generator or the STBRD command is used.
  SLEEP_TIME,     // Defines the waiting period in milliseconds after each received fragment.
  LOAD_TIME,      // Defines the waiting period in milliseconds before each time a new page is pushed.
  WAIT_TIME,      // Defines the waiting period for command responses in milliseconds.
  FEE_ADDRESS;    // Enforces that the data reading is carried out with the Start Block Read
                  // (STBRD) command.

uint64_t 
  BUFFER_SIZE,    // Size of the DMA buffer.
  INIT_WORD;      // Sets the second word of each fragment when the data generator is used.
  

typedef struct _descriptor_entry{
  uint32_t src_low;
  uint32_t src_high;
  uint32_t dst_low;
  uint32_t dst_high;
  uint32_t ctrl;
  uint32_t reservd1;
  uint32_t reservd2;
  uint32_t reservd3;
} descriptor_entry; 
descriptor_entry *descriptor, *descriptor_usr;

typedef struct configDataStruct{
  uint64_t dma_page_length;
  uint64_t buffer_size;
  uint64_t fifo_entries;
  uint64_t data_length;
  uint64_t data_generator;
  uint64_t loopback;
  uint64_t sw_fifo_offset;
  uint64_t data_offset;

  uint64_t pattern;
  uint64_t init_value;
  uint64_t init_word;
  uint64_t random_seed;
  uint64_t max_event;
  
  uint64_t reset_level;
  //uint64_t check_level;
  //uint64_t stat_file;
  uint64_t ddl_header;
  uint64_t no_rdyrx;
  //uint64_t min_fifo;
  //uint64_t max_fifo;
  uint64_t sleep_time;
  uint64_t load_time;
  uint64_t wait_time;
  uint64_t fee_address;
  //uint64_t binary;
  //char output_file[256];
}configData;
configData config_data;

/// \struct Card
/// \brief Struct for holding all object needed for DMA in one place.
/// 
typedef struct CardStruct{
  PciDevice *device;            ///< PDA struct for the device.
  int serial;                   ///< Serial number of the card.
  DMABuffer *buffer[6];         ///< PDA structs for the allocated buffers.
  DMABuffer_SGNode *sgnode[6];  ///< PDA structs for the SG lists of the buffers.
  uint32_t *map[6];             ///< Userspace addresses of the mapped buffers.

  Bar *bar[6];                  ///< PDA structs for the BARs.
  uint64_t length[6];           ///< Lengths of the BARs.
  uint32_t *barAddress[6];        ///< Userspace addresses of the mapped BARs

  uint32_t *swFifoAddress[6];         ///< Userspace addresses of the software FIFOs.
  uint32_t *dataStartAddress[6];      ///< Userspace addresses of the start of the data.
  int numberOfPages[6];          ///< Number of pages in the buffers.

} Card;

DeviceOperator *dop;

/// Finds attached cards, gets their types and initializes the function pointers.
/// \return  -1 in case of error, 0 otherwise.
int findCards();

/// Opens a channel on a card, and intializes the Card struct
/// \param card     Card to be assigned to the device we open.
/// \param channel  The number of the channel we want to access on the card.
/// \param serial   The serial number of the card we want to access.
/// \return         -1 in case of error, 0 otherwise.
int openCard(Card *card, int channel, int serial);

/// Allocates memory and assigns it to a channel.
/// \param card     The card we want to access.
/// \param channel  The channel of the card we want the buffer to be assigned to.
/// \return         -1 in case of error, 0 otherwise.
int allocateMemory(Card *card, int channel);

/// Closes card and every channel on it.
/// \param card  The card we want to close.
/// \return      -1 in case of error, 0 otherwise.
int closeCard(Card *card);

/// Closes channel.
/// \param card     The card we want to close.
/// \param channel  The channel we want to close.
/// \return         -1 in case of error, 0 otherwise.
int closeChannel(Card *card, int channel);

/// Prepares the card and starts the DMA.
/// \param card     The card we want to access.
/// \param channel  The channel on which we want to perform DMA.
/// \return         -1 in case of error, 0 otherwise.
int startDMA(Card *card, int channel);

/// Stops DMA.
/// \param card     The card we want to access.
/// \param channel  The channel on which we performed DMA.
/// \return         -1 in case of error, 0 otherwise.
int stopDMA(Card *card, int channel);

/// Prepares the card and starts the dummy DMA (DMA simulated by software).
/// \param card     The card we want to access.
/// \param channel  The channel on which we want to perform dummy DMA.
/// \return         -1 in case of error, 0 otherwise.
int startDummyDMA(Card *card, int channel, int serial);

/// Stops dummy DMA.
/// \param card     The card we want to access.
/// \param channel  The channel on which we performed dummy DMA.
/// \return         -1 in case of error, 0 otherwise.
int stopDummyDMA(Card *card, int channel);

/// Checks if data has been written to the current page and if so, does the necessary steps.
/// \param card      The card we want to access.
/// \param channel   The channel where we perform the DMA.
/// \param index_wr  Pointer to the index of the entry to which the current page has been assigned.
/// \param pushed    Array where the readout proccess keeps track of incoming data.
/// \return          0 if data has arrived, 1 otherwise.
int checkPageWritten(Card *card, int channel, int *index_wr, int *pushed);

/// Checks if the data on the current page has been read out and is so, pushes new page
/// \param card       The card we want to access.
/// \param channel    The channel where we perform the DMA.
/// \param index_wr   Pointer to the index of the entry to which the current page has been assigned.
/// \param pushed     Array where the readout proccess keeps track of incoming data.
/// \param next_page  Pointer to the index that keeps track of the next page to be pushed.
/// \return           0 if data has been readout, 1 otherwise.
int checkPageRead(Card *card, int channel, int *index_rd, int *pushed, int *next_page);

/// Initializes the function pointers according to the cards found in the machine, 
/// \param ids  List of strings vendor and device IDs of the cards found.
/// \return     -1 in case of error, 0 otherwise.
int initCard(char *ids[]);

/*********************/
/* Function pointers */
/*********************/

// TODO 
// - rewriting u_pointers to getMap
// - changing function arguments to Card

/// Sets the starting addresses of the sofware FIFO and the data according to the offset parameters.
/// \param buff           The DMA buffer where we want to do the DMA
/// \param swFifoAddress  Pointer to the starting address of the software FIFO. 
/// \param dataStartAddress     Pointer to the starting address of the data. 
/// \return               -1 in case of error, 0 otherwise.
int (*setAddress)(DMABuffer *, uint32_t**, uint32_t**);
int setAddress_crorc(DMABuffer *buff, uint32_t** swFifoAddress,  uint32_t** dataStartAddress);
int setAddress_cru(DMABuffer *buff, uint32_t** swFifoAddress, uint32_t** dataStartAddress);

/// Initializes the software FIFO.
/// \param swFifoAddress  The starting address of the software FIFO.
/// \return               -1 in case of error, 0 otherwise.
int (*initSwFifo)(uint32_t*);
int initSwFifo_crorc(uint32_t* swFifoAddress);
int initSwFifo_cru(uint32_t* swFifoAddress);

/// Pushes pages to the firmware FIFO.
/// \param buff           The DMA buffer where we want to do the DMA.
/// \param barAddress       The starting address of the BAR.
/// \param pagesToPush  The number of pages we want to push (starting from the 0th).
/// \return               -1 in case of error, 0 otherwise.
int (*initFwFifo)(DMABuffer*, uint32_t*, int);
int initFwFifo_crorc(DMABuffer* buff, uint32_t* barAddress, int pagesToPush);
int initFwFifo_cru(DMABuffer* buff, uint32_t* barAddress, int pagesToPush);

/// Pushes new page.
/// \param barAddress       The starting address of the BAR.
/// \param page_addr      The bus address of the page we want to push.
/// \param sw_fifo_index  The index of the software FIFO entry to which the page will be assigned.
/// \return               -1 in case of error, 0 otherwise.
int (*pushNewPage)(uint32_t*, uint64_t*, int);
int pushNewPage_crorc(uint32_t* barAddress, uint64_t* page_addr, int sw_fifo_index);
int pushNewPage_cru(uint32_t* barAddress, uint64_t* page_addr, int sw_fifo_index);

/// Resets the card according to the RESET_LEVEL parameter
/// \param barAddress     The starting address of the BAR.
/// \param reset_level  RESET_LEVEL parameter.
/// \return             -1 in case of error, 0 otherwise.
int (*resetCard)(uint32_t*, int);
int resetCard_crorc(uint32_t* barAddress, int reset_level);
int resetCard_cru(uint32_t* barAddress, int reset_level);

/// Reads the value of a register.
/// \param barAddress  The starting address of the BAR.
/// \param index     The index of the register we want to read.
/// \return          The value of the register. 
uint32_t readReg(uint32_t* barAddress, int index);

/// Writes to a register.
/// \param barAddress  The starting address of the BAR.
/// \param index     The index of the register we want to read.
/// \param value     The value we want to write.
/// \return          The value of the register. 
uint32_t writeReg(uint32_t* barAddress, int index, int value);

int (*dataArrived)(uint32_t*, int);
int dataArrived_crorc(uint32_t* swFifoAddress, int index);
int dataArrived_cru(uint32_t* swFifoAddress, int index);

/// Converts page index to bus address.
/// \param page  Index of the page.
/// \param buff  The buffer where the page is.
/// \return      The bus address of the page. 
uint64_t* pageToDAddress(int page, DMABuffer* buff);

/// Converts page index to userspace address.
/// \param page  Index of the page.
/// \param buff  The buffer where the page is.
/// \return      The userspace address of the page. 
uint32_t* pageToAddress(int page, DMABuffer* buff);

//DMABuffer_SGNode* pageToSGnode(int page, DMABuffer* buff);
//int addressToPage(DMABuffer* buff, uint32_t* addr);

/// Gets the number of pages available in a given buffer and 
/// sets the corresponding struct member accordingly.
/// \param card     The card we want to access. 
/// \param channel  The channel of the buffer.
/// \return         The number of the available pages.
int (*getNumOfPages)(Card *card, int channel);
int getNumOfPages_crorc(Card *card, int channel);
int getNumOfPages_cru(Card *card, int channel);

/// Arms the data generator of the card.
/// \param barAddress  The starting address of the BAR.
/// \return          -1 in case of error, 0 otherwise.
int (*armDataGenerator)(uint32_t*);
int armDataGenerator_crorc(uint32_t* barAddress);
int armDataGenerator_cru(uint32_t* barAddress);

/// Starts the data generator of the card.
/// \param barAddress  The starting address of the BAR.
/// \return          -1 in case of error, 0 otherwise.
int (*startDataGenerator)(uint32_t*);
int startDataGenerator_crorc(uint32_t* barAddress);
int startDataGenerator_cru(uint32_t* barAddress);

/// Sets the card to be able to receive data.
/// \param buff       The buffer where we want to do the DMA.
/// \param barAddress   The starting address of the BAR.
/// \return           -1 in case of error, 0 otherwise.
int (*startDataReceiving)(DMABuffer*, uint32_t*);
int startDataReceiving_crorc(DMABuffer* buff, uint32_t* barAddress);
int startDataReceiving_cru(DMABuffer* buff, uint32_t* barAddress);

/// Sets back a given software FIFO entry to its initial state.
/// \param swFifoAddress  The starting address of the software FIFO.
/// \param index         The index of the software FIFO entry we want to set back
/// \return             -1 in case of error, 0 otherwise.
int (*setSwFifoEntry)(uint32_t*, int);
int setSwFifoEntry_crorc(uint32_t *swFifoAddress, int index);
int setSwFifoEntry_cru(uint32_t* swFifoAddress, int index);

/// Reads paramters from the config file and sets variables accordingly.
/// \param config_file  String of the path to the configuration file.
/// \param config_data  Struct for configuration data.
/// \return             -1 in case of error, 0 otherwise.
int (*configure)(char*, configData*);
int configure_crorc(char *config_file, configData *config_data);
int configure_cru(char *config_file, configData *config_data);

/*
int (*checkIfFifoEmpty)(uint32_t*);
writePageToFile
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif


