/// \file header_cru.c
/// \brief C file containing the function definitions for the CRU.
///
///  Last updated: 26/04/2016
///
/// \author Tuan Mate Nguyen <tuan.mate.nguyen@cern.ch>

#include "header.h"


int configure_cru(char *config_file, configData *config_data)
{
  return 0;
}

int resetCard_cru(uint32_t* bar_addr, int reset_level)
{
  //just write a specific value to a specific register
  return 0;
}

int setAddress_cru(DMABuffer *buff, uint32_t** sw_fifo_start, uint32_t** data_start)
{
  DMABuffer_SGNode *sgnode;

  if(DMABuffer_getSGList(buff, &sgnode) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    return -1;
  }
  
  if(sgnode->length < SW_FIFO_OFFSET + (128 + 128*8)*sizeof(uint32_t) + DATA_OFFSET){
    printf("Offset is too big\n");
    return -1;
  }

  //FULL_OFFSET = SW_FIFO_OFFSET + (128 + 128*8)*sizeof(uint32_t) + DATA_OFFSET;

  *sw_fifo_start = (uint32_t*)sgnode->u_pointer + SW_FIFO_OFFSET/sizeof(uint32_t);

  if(sgnode->length >= FULL_OFFSET + DMA_PAGE_LENGTH){
    *data_start = (uint32_t*)sgnode->u_pointer + FULL_OFFSET/sizeof(uint32_t);
  }
  else{
    sgnode = sgnode->next;
    *data_start = (uint32_t*)sgnode->u_pointer;
    DATA_OFFSET = DATA_STARTS_ON_NEW_NODE;
  }

  return 0;
}

int initSwFifo_cru(uint32_t* sw_fifo_start)
{
  int i;
  
  for(i = 0; i < 128; i++){
    *(sw_fifo_start+i) = 0;//status
  }

  return 0;
}

int initFwFifo_cru(DMABuffer* buff, uint32_t* bar_addr, int pages_to_push)
{
  int i;
  uint32_t ctrl;
  uint32_t* status;
  DMABuffer_SGNode *sgnode;

  for(i = 0; i < pages_to_push; i++){
    ctrl = 0;
    ctrl = i << 18;
    ctrl += DMA_PAGE_LENGTH/4;
    descriptor_usr[i].ctrl = ctrl;
    /** Adresses in the card's memory (DMA source) */
    descriptor_usr[i].src_low = i*DMA_PAGE_LENGTH;
    descriptor_usr[i].src_high = 0x0;
    /** Addresses in the RAM (DMA destination) */
    descriptor_usr[i].dst_low = (uint64_t)(pageToDAddress(i, buff)) & 0xffffffff;
    descriptor_usr[i].dst_high = (uint64_t)(pageToDAddress(i, buff)) >> 32;
    /*fill the reserved bit with zero*/
    descriptor_usr[i].reservd1 = 0x0;
    descriptor_usr[i].reservd2 = 0x0;
    descriptor_usr[i].reservd3 = 0x0;
  }
  
  if(DMABuffer_getSGList(buff, &sgnode) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    return -1;
  }
  status = (uint32_t*)sgnode->d_pointer + SW_FIFO_OFFSET;

  /** Status base address, low and high part, respectively */
  writeReg(bar_addr, 0, (uint64_t)status & 0xffffffff);
  writeReg(bar_addr, 1, (uint64_t)status >> 32);
     
  /** Destination (card's memory) addresses, low and high part, respectively */
  writeReg(bar_addr, 2, 0x8000);
  writeReg(bar_addr, 3, 0x0);

  /** Set descriptor table size, same as number of available pages-1 */
  writeReg(bar_addr, 5, FIFO_ENTRIES-1); 
  
  return 0;
}

int pushNewPage_cru(uint32_t* bar_addr, uint64_t*  page_addr, int sw_fifo_index)
{
  //Simply write some register?
  return 0;
} 

int dataArrived_cru(uint32_t* sw_fifo_addr, int index)
{
  return sw_fifo_addr[index];
}

int startDataReceiving_cru(DMABuffer* buff, uint32_t* bar_addr)
{
  /** Set status to send status for every page not only for the last one */ 
  writeReg(bar_addr, 6, 0x1);

  return 0;
}

int armDataGenerator_cru(uint32_t* bar_addr)
{
  /* make pcie ready signal*/
  writeReg(bar_addr, 129, 0x1); 

  return 0;
}

int startDataGenerator_cru(uint32_t* bar_addr)
{
  /** Programming the user module to trigger the data emulator */
  writeReg(bar_addr, 128, 0x1);

  return 0;
}

int setSwFifoEntry_cru(uint32_t* sw_fifo_addr, int index){
  sw_fifo_addr[index] = 0;

  return 0;
}

int getNumOfPages_cru(Card *card, int channel)
{
  size_t length;

  if(DMABuffer_getLength(card->buffer[channel], &length) != PDA_SUCCESS ){
    printf("Can't get length ...\n");
    exit(EXIT_FAILURE);
  }

  if(DATA_OFFSET == DATA_STARTS_ON_NEW_NODE){
    return (length - card->sgnode[channel]->length)/DMA_PAGE_LENGTH;
  }
  else
    return (card->sgnode[channel]->length - FULL_OFFSET)/DMA_PAGE_LENGTH + (length - card->sgnode[channel]->length)/DMA_PAGE_LENGTH;
}
