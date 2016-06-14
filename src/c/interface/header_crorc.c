/// \file header_crorc.c
/// \brief C file containing the function definitions for the CRORC.
///
///  Last updated: 26/04/2016
///
/// \author Tuan Mate Nguyen <tuan.mate.nguyen@cern.ch>

#include "header.h"

int configure_crorc(char *config_file, configData *config_data)
{
  FILE *fp;
  fp = fopen(config_file, "r");
  if(fp == NULL){
    printf("Can't open config file\n");
    return -1;
  }
  char line[256];
  char *token;
  int struct_member = 0;

  // Parsing the configuration file.
  while(fgets(line, 256, fp) != NULL){
    token = strstr(line, "=");

    if(line[0] == '#' || token == NULL){
      continue;
    }

    token = token + strlen("=");
    strtok(token, "\n");
    strtok(line, "=");

    // Read configuration data to the configuration struct.
    *(((uint64_t*)config_data)+struct_member) = strtoull(token,NULL,0);
    
    struct_member++;
  }
  // Close configuration file.
  fclose(fp);

  // Initialize global paramters based on the configuration data.
  DMA_PAGE_LENGTH = config_data->dma_page_length;
  BUFFER_SIZE = config_data->buffer_size;
  FIFO_ENTRIES = config_data->fifo_entries;
  DATA_LENGTH = config_data->data_length;
  DATA_GENERATOR = config_data->data_generator;
  LOOPBACK = config_data->loopback;
  SW_FIFO_OFFSET = config_data->sw_fifo_offset;
  DATA_OFFSET = config_data->data_offset;
  FULL_OFFSET = SW_FIFO_OFFSET + FIFO_ENTRIES*8 + DATA_OFFSET;
  PATTERN = config_data->pattern;
  INIT_VALUE = config_data->init_value;
  INIT_WORD = config_data->init_word;
  RANDOM_SEED = config_data->random_seed;
  MAX_EVENT = config_data->max_event;
  RESET_LEVEL = config_data->reset_level;
  DDL_HEADER = config_data->ddl_header;
  SLEEP_TIME = config_data->sleep_time;
  LOAD_TIME = config_data->load_time;
  WAIT_TIME = config_data->wait_time;
  FEE_ADDRESS = config_data->fee_address;
  if(DATA_GENERATOR || FEE_ADDRESS){
    NO_RDYRX = 1;
  }
  else{
    //If data generator or FEE_ADDRESS: NO_RDYRX = 1, else:
    NO_RDYRX = config_data->no_rdyrx;
  }
  return 0;
}

int resetCard_crorc(uint32_t* barAddress, int reset_level)
{
  if (reset_level)
  {
    rorcArmDDL(barAddress, RORC_RESET_RORC);
  }

  if ((reset_level > 1) && (LOOPBACK < 3))
  {
    rorcArmDDL(barAddress, RORC_RESET_DIU);
    printf(" DIU reset\n");

    if ((reset_level > 2) && (LOOPBACK != 1))
    {    
      // Wait a little before SIU reset.
      usleep(100000);
      // Reset SIU.
      if (rorcArmDDL(barAddress, RORC_RESET_SIU) != RORC_STATUS_OK)
      {
        printf("Error: Could not reset SIU. I stop.\n");
        return -1;
      }
      rorcArmDDL(barAddress, RORC_RESET_DIU);
    }

    rorcArmDDL(barAddress, RORC_RESET_RORC);
  }

  if (reset_level)
  {
    // Wait a little after reset.
    usleep(100000);
  }

  return 0;
}

int setAddress_crorc(DMABuffer *buff, uint32_t** swFifoAddress, uint32_t** data_start)
{
  DMABuffer_SGNode *sgnode;

  // Getting the first node of the buffer.
  if(DMABuffer_getSGList(buff, &sgnode) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    return -1;
  }

  // If the first node's size is too small for the FIFO and offsets return with error.
  if(sgnode->length < SW_FIFO_OFFSET + 128*8 + DATA_OFFSET){
    printf("Offset is too big\n");
    return -1;
  }

  // Setting the start of the software FIFO.
  *swFifoAddress = (uint32_t*)sgnode->u_pointer + SW_FIFO_OFFSET/sizeof(uint32_t);

  // If there is space on the first node, setting data start, otherwise jump to the next node 
  // and setting it there.
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

int initSwFifo_crorc(uint32_t* swFifoAddress)
{
  int i;
  // Initializing the software FIFO entries to their inital state (no data).
  for(i = 0; i < FIFO_ENTRIES; i++){
    *(swFifoAddress + 2*i) = -1;  //length
    *(swFifoAddress + 2*i+1) = -1;//status
  }
 
  return 0;
}

int initFwFifo_crorc(DMABuffer* buff, uint32_t* barAddress, int pagesToPush)
{
  int i;
  // Pushing a given number of pages to the firmware FIFO.
  for(i = 0; i < pagesToPush; i++){
    pushNewPage_crorc(barAddress, pageToDAddress(i, buff), i);
  }
  return 0;
}

int pushNewPage_crorc(uint32_t* barAddress, uint64_t* page_addr, int sw_fifo_index)
{
  // Push a page (pass the bus address and the software FIFO index to a register).
  rorcPushRxFreeFifo (barAddress, (unsigned long)page_addr, DMA_PAGE_LENGTH/4, sw_fifo_index);
  
  return 0;
} 

int dataArrived_crorc(uint32_t* swFifoAddress, int index)
{
  // If whole event has arrived returns 2, if part of event arrived to a page returns 1,
  // if no data has arrived returns 0.
  return rorcHasData(swFifoAddress, index);
}

int startDataReceiving_crorc(DMABuffer* buff, uint32_t* barAddress)
{
  DMABuffer_SGNode *sgnode;

  setLoopPerSec(&loop_per_usec, &pci_loop_per_usec, barAddress);
  ddlFindDiuVersion(barAddress);

  // Preparing the card.
  if(LOOPBACK == 2){

    resetCard(barAddress, 3);

    if (rorcCheckLink(barAddress) != RORC_STATUS_OK){
      printf("SIU not seen. Can not clear SIU status");
      return -1;
    }
    else{
      if(ddlReadSiu(barAddress, 0, DDL_RESPONSE_TIME) == -1){
	printf("SIU read error\n");
	return -1;
      }
    }   
    if(ddlReadDiu(barAddress, 0, DDL_RESPONSE_TIME) == -1){
      printf("DIU read error\n");
      return -1;
    }
  }//loopback == 2

  resetCard(barAddress, 1);
  rorcReset(barAddress, RORC_RESET_FF);  
  
  // Checking if firmware FIFO is empty.
  if (rorcCheckRxFreeFifo(barAddress) == RORC_FF_EMPTY){
    //  printf("freeFifo emptied\n");
  }
  else{
    printf("Error: freeFifo is not empty, %d\n", rorcCheckRxFreeFifo(barAddress));
    return -1;
  }

  if(DMABuffer_getSGList(buff, &sgnode) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    exit(EXIT_FAILURE);
  }

  // Starting data receiver.
  rorcStartDataReceiver(barAddress, (volatile unsigned long)((volatile unsigned long*)sgnode->d_pointer+SW_FIFO_OFFSET/sizeof(unsigned long)));

  return 0;
}

int armDataGenerator_crorc(uint32_t* barAddress)
{
  int ret, rounded_len;
  stword_t stw;

  /** If data stream comes from external data generator */
  if(LOOPBACK == 0){
    ret = rorcStartTrigger(barAddress, DDL_RESPONSE_TIME * pci_loop_per_usec, &stw);
    if(ret == RORC_LINK_NOT_ON){
      printf("Error: LINK IS DOWN, RDYRX command can not be sent. I stop\n");
      exit(EXIT_FAILURE);
    }
    if (ret == RORC_STATUS_ERROR){
      printf("Error: RDYRX command can not be sent. I stop\n");
      exit(EXIT_FAILURE);
    }
    if (ret == RORC_NOT_ACCEPTED){
      printf("No reply arrived for RDYRX in timeout %d usec\n", DDL_RESPONSE_TIME); 
      exit(EXIT_FAILURE);
    }
    else
      printf("FEE accepted the RDYDX command. Its reply: 0x%08lx\n\n",stw.stw);
  }//LOOPBACK == 0

  if(rorcArmDataGenerator(barAddress, INIT_VALUE, INIT_WORD, PATTERN, 
			  DATA_LENGTH/4, RANDOM_SEED, &rounded_len) != RORC_STATUS_OK){
    printf("Can't arm data generator\n");
    return -1;
  }

  // If loopback is inside the RORC.
  if (LOOPBACK == 3){
    rorcParamOn(barAddress, PRORC_PARAM_LOOPB);
    usleep(100000);
  } 
 
  // If loopback is inside the SIU.
  if (LOOPBACK == 2){
    ret = ddlSetSiuLoopBack(barAddress, DDL_RESPONSE_TIME * pci_loop_per_usec, &stw);
    if (ret != RORC_STATUS_OK){
      printf("Error: SIU loopback error\n");
      return -1;
    }
    usleep(100000);
    if (rorcCheckLink(barAddress) != RORC_STATUS_OK){
      printf("SIU not seen. Can not clear SIU status\n");
      return -1;
    }
    else{
      if(ddlReadSiu(barAddress, 0, DDL_RESPONSE_TIME) == -1){
	printf("SIU read error\n");
	return -1;
      }
    }
    if(ddlReadDiu(barAddress, 0, DDL_RESPONSE_TIME) == -1){
      printf("DIU read error\n");
      return -1;
    }
  }//loopback == 2

  return 0;
}

int startDataGenerator_crorc(uint32_t* barAddress)
{
  rorcStartDataGenerator(barAddress, MAX_EVENT);
  fflush (stdout);

  return 0;
}

int setSwFifoEntry_crorc(uint32_t *swFifoAddress, int index){

  // Setting a given entry of the software FIFO back to its initial state.
  swFifoAddress[2*index] = -1;
  swFifoAddress[2*index+1] = -1;

  return 0;
}

int getNumOfPages_crorc(Card *card, int channel)
{
  size_t length;

  // Get buffer size.
  if(DMABuffer_getLength(card->buffer[channel], &length) != PDA_SUCCESS ){
    printf("Can't get length ...\n");
    exit(EXIT_FAILURE);
  }
 
  // If data starts only on the second node, substract the size of the first node.
  if(DATA_OFFSET == DATA_STARTS_ON_NEW_NODE){
    card->numberOfPages[channel] = (length - card->sgnode[channel]->length)/DMA_PAGE_LENGTH;
    return (length - card->sgnode[channel]->length)/DMA_PAGE_LENGTH;
  }
  // Otherwise substract only the offset, and see how many pages fit then to the buffer.
  else{
    card->numberOfPages[channel] = (card->sgnode[channel]->length - FULL_OFFSET)/DMA_PAGE_LENGTH + (length - card->sgnode[channel]->length)/DMA_PAGE_LENGTH;
    return (card->sgnode[channel]->length - FULL_OFFSET)/DMA_PAGE_LENGTH + (length - card->sgnode[channel]->length)/DMA_PAGE_LENGTH;
  }
}
