/// \file header_common.c
/// \brief C file containing function definitions that are the same for both the CRORC and CRU.
///
///  Last updated: 26/04/2016
///
/// \author Tuan Mate Nguyen <tuan.mate.nguyen@cern.ch>


#include "header.h"
#include <dirent.h>
#include <string.h>


int initCard(char **ids)
{
  DIR *directory = NULL;         // Pointer to the directory that lists the PCI devices.
  struct dirent *file;           // For iterating through the PCI devices.
  FILE *vendorFp, *deviceFp;   // File pointer to the 'vendor' and 'device' files.
  char device[51],               // String of the 'device' file path.
       vendor[51],               // String of the 'vendor' file path. 
       deviceId[6],             // String of the device ID (found in the 'device' file).
       vendorId[6];             // String of the vendor ID (found in the 'vendor' file).
       //fullId[10];              // Vendor ID and device ID together.
  //char cruId[] = "e001";        // Device ID of the CRU cards.
  char crorcId[] = "0033";      // Device ID of the CRORC cards.
  char cernId[] = "0x10dc";     // Vendor ID of CERN devices.

  // Opening the folder of the PCI devices.
  directory = opendir("/sys/bus/pci/devices/");
  if(directory == NULL){
    printf("Can't open directory\n");
  }

  // Iterating through the devices to find the cards.
  while(NULL != (file = readdir(directory))) {

    // Opening the 'vendor' file.
    snprintf(vendor, sizeof(vendor), "%s%s/vendor", "/sys/bus/pci/devices/", file->d_name);
    vendorFp = fopen(vendor, "r");
    if(vendorFp == NULL){
      continue;
    }

    // Getting the vendor ID from the 'vendor' file.
    fscanf(vendorFp, "%s", vendorId);

    // Comparing the vendor ID with the CERN vendor ID
    if(strcmp(vendorId, cernId) == 0){

      // Opening the 'device' file.
      snprintf(device, sizeof(device), "%s%s/device", "/sys/bus/pci/devices/", file->d_name);
      //printf("%s\n", device);
      deviceFp = fopen(device, "r");
      if(deviceFp == NULL){
	printf("Can't open device file\n");
	return -1;
      }

      // Getting the device ID from the 'device' file.
      fscanf(deviceFp, "%s", deviceId);
      printf("Device ID: %s\n", deviceId);

      // Cutting the starting '0x' off from the string of the IDs.
      memmove(deviceId, deviceId+2, strlen(deviceId)-1);
      memmove(vendorId, vendorId+2, strlen(vendorId)-1);

      // Opening the 'config' file to get the revision ID.
      snprintf(device, sizeof(device), "%s%s/config", "/sys/bus/pci/devices/", file->d_name);
      FILE *cfp = fopen(device, "rb");
      char cfg[64];

      fread(&cfg, 1, sizeof(cfg), cfp);
      /*for(int j = 0; j<sizeof(cfg);j++){
	printf("%08x ", cfg[j]);
	if((j+1)%4 == 0)
	  printf("\n");
      }*/      
      //fseek(cfp, 8, SEEK_SET);
      //fread(&cfg, 1, sizeof(cfg), cfp);

      // If device ID matches with the CRORC ID it's a CRORC.
      if(strcmp(deviceId, crorcId) == 0){
	//The card is a CRORC
	printf("The card is a CRORC\n");

	// Initializing the function pointers.
	setAddress = &setAddress_crorc;
	initSwFifo = &initSwFifo_crorc;
	initFwFifo = &initFwFifo_crorc;
	pushNewPage = &pushNewPage_crorc;
	dataArrived = &dataArrived_crorc;
	armDataGenerator = &armDataGenerator_crorc;
	startDataGenerator = &startDataGenerator_crorc;
	startDataReceiving = &startDataReceiving_crorc;
	setSwFifoEntry = &setSwFifoEntry_crorc;
	getNumOfPages = &getNumOfPages_crorc;
	configure = &configure_crorc;
	resetCard = &resetCard_crorc;
	//snprintf(fullId, sizeof(fullId), "%s %s", vendorId, deviceId);
	//ids[0] = (const char*)fullId;
	//printf("%s ", fullId);
	ids[0] = "10dc 0033";
	ids[1] = NULL;
      }
      // Otherwise it's a CRU.
      else{
	//The card is a CRU
	printf("The card is a CRU\n");

	// Initializing the function pointers.
	setAddress = &setAddress_cru;
	initSwFifo = &initSwFifo_cru;
	initFwFifo = &initFwFifo_cru;
	pushNewPage = &pushNewPage_cru;
	dataArrived = &dataArrived_cru;
	armDataGenerator = &armDataGenerator_cru;
	startDataGenerator = &startDataGenerator_cru;
	startDataReceiving = &startDataReceiving_cru;
	setSwFifoEntry = &setSwFifoEntry_cru;
	getNumOfPages = &getNumOfPages_cru;
	configure = &configure_cru;
	resetCard = &resetCard_cru;
	ids[0] =  "10dc e001";
	ids[1] = NULL;
      }
      fclose(deviceFp);
      fclose(vendorFp);
      break;
    }
    fclose(vendorFp);
   
  }//Only one card in one machine?

  // Close the directory.
  closedir(directory);
  
  return 0;
}

uint32_t readReg(uint32_t* barAddress, int index)
{
  return *(barAddress + index);
}

uint32_t writeReg(uint32_t* barAddress, int index, int value)
{
  *(barAddress + index) = value;

  // Returning the new register value.
  return *(barAddress + index);
}

uint64_t* pageToDAddress(int page, DMABuffer* buff)
{
  DMABuffer_SGNode *sgnode;
  int i, node_pages;
  uint64_t* page_addr;

  // Getting the first SG node of the buffer. 
  if(DMABuffer_getSGList(buff, &sgnode) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    exit(EXIT_FAILURE);
  }
  
  // Setting the address to the first page's address and 
  // getting the number of pages on the current node.
  if(DATA_OFFSET == DATA_STARTS_ON_NEW_NODE){
    sgnode = sgnode->next;
    page_addr = (uint64_t*)sgnode->d_pointer;
    node_pages = sgnode->length/DMA_PAGE_LENGTH;
  }  
  else{
    page_addr = (uint64_t*)sgnode->d_pointer + FULL_OFFSET/sizeof(uint64_t);
    node_pages = (sgnode->length-FULL_OFFSET)/DMA_PAGE_LENGTH;
  }

  i = 0;

  // Finding the node containing the page that we are looking for.  
  if(page < node_pages)
    return page_addr+page*DMA_PAGE_LENGTH/sizeof(uint64_t);

  while(i + node_pages < page+1){
    i += node_pages;
    sgnode = sgnode->next;
    node_pages = sgnode->length/DMA_PAGE_LENGTH;
  }
  
  // Getting the bus address of the page and returning it.
  page_addr = (uint64_t*)sgnode->d_pointer + (page-i)*DMA_PAGE_LENGTH/sizeof(uint64_t);

  return page_addr;
}

uint32_t* pageToAddress(int page, DMABuffer* buff)
{
  DMABuffer_SGNode *sgnode;
  int i, node_pages;
  uint32_t* page_addr;

  // Getting the first SG node of the buffer. 
  if(DMABuffer_getSGList(buff, &sgnode) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    exit(EXIT_FAILURE);
  }

  // Setting the address to the first page's address and 
  // getting the number of pages on the current node.
  if(DATA_OFFSET == DATA_STARTS_ON_NEW_NODE){
    sgnode = sgnode->next;
    page_addr = (uint32_t*)sgnode->u_pointer;
    node_pages = sgnode->length/DMA_PAGE_LENGTH;
  }  
  else{
    page_addr = (uint32_t*)sgnode->u_pointer + FULL_OFFSET/sizeof(uint32_t);
    node_pages = (sgnode->length-FULL_OFFSET)/DMA_PAGE_LENGTH;
  }

  i = 0;
  
  // Finding the node containing the page that we are looking for.
  if(page < node_pages)
    return page_addr+page*DMA_PAGE_LENGTH/sizeof(uint32_t);

  while(i + node_pages < page+1){
    i += node_pages;
    sgnode = sgnode->next;
    node_pages = sgnode->length/DMA_PAGE_LENGTH;
  }
  
 // Getting the userspace address of the page and returning it.
  page_addr = (uint32_t*)sgnode->u_pointer + (page-i)*DMA_PAGE_LENGTH/sizeof(uint32_t);

  return page_addr;
}

DMABuffer_SGNode* pageToSGnode(int page, DMABuffer* buff)
{
  DMABuffer_SGNode *sgnode;
  int i, node_pages;

  // Getting the first SG node of the buffer.
  if(DMABuffer_getSGList(buff, &sgnode) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    exit(EXIT_FAILURE);
  }

  i = 0;

  // Finding the node containing the page.
  node_pages = (sgnode->length-FULL_OFFSET)/DMA_PAGE_LENGTH;

  while(i + node_pages < page+1){
    i += node_pages;
    sgnode = sgnode->next;
    node_pages = sgnode->length/DMA_PAGE_LENGTH;
  }

  // Returning the SG node.
  return sgnode;
}

int addressToPage(DMABuffer* buff, uint32_t* addr)
{
  DMABuffer_SGNode *sgnode;
  uint32_t *start, *end;
  uint32_t page;

  // Getting the first SG node of the buffer.
  if(DMABuffer_getSGList(buff, &sgnode) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    exit(EXIT_FAILURE);
  }

  // Getting the starting and ending address of the first node that contains data.
  if(DATA_OFFSET == DATA_STARTS_ON_NEW_NODE){
    sgnode = sgnode->next;
    start = (uint32_t*)sgnode->u_pointer;
    end = (uint32_t*)sgnode->u_pointer + sgnode->length/sizeof(uint32_t);
  }
  else{
    start = (uint32_t*)sgnode->u_pointer + FULL_OFFSET;
    end = (uint32_t*)sgnode->u_pointer + sgnode->length/sizeof(uint32_t);
  }
  page = 0;

  // If the address is between the starting and ending address of the node,
  // the page is on that node.
  while(addr > end){
    page += ((uint64_t)end-(uint64_t)start)/DMA_PAGE_LENGTH;
    sgnode = sgnode->next;
    if(sgnode == NULL){
      printf("Address is not in the SG list\n");
      exit(EXIT_FAILURE);
    }
    start = (uint32_t*)sgnode->u_pointer;
    end = (uint32_t*)sgnode->u_pointer + sgnode->length/sizeof(uint32_t);
  }

  // Getting the page number and returning it. 
  page += ((uint64_t)addr-(uint64_t)start)/DMA_PAGE_LENGTH;

  return page;
}

int findCards()
{
  char *ids[2];

  /** Getting the type of the cards, initialize function pointers accordingly */
  if(initCard(ids) != 0){
    printf("Can't get type of card\n");
    return -1;
  }

  /** Getting the parameters from the config file */
  configure("/root/pda/crorc/interface/config.txt", &config_data);

  // Checinkg if the PDA module (uio_pci_dma.ko) is inserted.
  if(PDAInit() != PDA_SUCCESS){
    printf("Error while initialization!\n");
    abort();
  }

  /** Accessing devices listed in ids */
  dop = DeviceOperator_new((const char**)ids, PDA_ENUMERATE_DEVICES );
  if(dop == NULL){
    printf("Unable to get device-operator!\n");
    return -1;
  }

  return 0;  
}

int openCard(Card *card, int channel, int serial)
{
  // Setting the serial number.
  card->serial = serial;

  // Getting the card.
  if(PDA_SUCCESS != DeviceOperator_getPciDevice(dop, &card->device, serial) ){
    if(PDA_SUCCESS != DeviceOperator_delete(dop, PDA_DELETE ) ){
      printf("Device generation totally failed!\n");
      return -1;
    }
    printf("Can't get device!\n");
    return -1;
  }
    
  /** Getting the BAR struct */
  if(PciDevice_getBar(card->device, &card->bar[channel], channel) != PDA_SUCCESS ){
    printf("Can't get bar ...\n");
    return -1;
  }

  /** Mapping the BAR starting  address */
  if(Bar_getMap(card->bar[channel], (void**)&card->barAddress[channel], &card->length[channel]) != PDA_SUCCESS ){
    printf("Can't get map ...\n");
    return -1;
  }

  return 0;
}

int allocateMemory(Card *card, int channel)
{
  /** Allocating buffer, getting the corresponding DMABuffer struct */
  card->buffer[channel] = NULL;
  if (PDA_SUCCESS != PciDevice_allocDMABuffer(card->device, channel, BUFFER_SIZE, &card->buffer[channel])){
    if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) ){
      printf("Buffer allocation totally failed!\n");
      return -1;
    }
    printf("DMA Buffer allocation failed!\n");
    return -1;
  }
  
  // Getting the first SG node from the buffer's SG list.
  if(DMABuffer_getSGList(card->buffer[channel], &card->sgnode[channel]) != PDA_SUCCESS ){
    printf("Can't get list ...\n");
    return -1;
  }

  // Mapping the start of the buffer to userspace.
  if(PDA_SUCCESS != DMABuffer_getMap(card->buffer[channel], (void*)&card->map[channel])){
    return -1;
  }

  // Getting the number of pages in the buffer.
  getNumOfPages(card, channel);

  return 0;
}

int closeCard(Card *card)
{
  int i;
  // PDA function to clean up and delete device.
  PciDevice_delete(card->device, PDA_DELETE);
  card->device = NULL;
  card->serial = 0;
  for(i = 0; i < 6; i++){
    card->buffer[i] = NULL;
    card->sgnode[i] = NULL;
    card->map[i] = NULL;
    card->bar[i] = NULL;
    card->length[i] = 0;
    card->barAddress[i] = NULL;
    card->swFifoAddress[i] = NULL;
    card->dataStartAddress[i] = NULL;
    card->numberOfPages[i] = 0;
  }

  return 0;
}

int closeChannel(Card *card, int channel)
{
  // PDA function to deallocate buffer.
  PciDevice_deleteDMABuffer(card->device, card->buffer[channel]);
  return 0;
}

int startDMA(Card *card, int channel)
{
  int ret;
  stword_t stw;
  uint32_t *dataStartAddress;

  // For random sleep time after every received data block.
  if(SLEEP_TIME || LOAD_TIME){
    srand(time(NULL));
  }

  // Setting (and getting) the starting address of the software FIFO and the data
  setAddress(card->buffer[channel], &card->swFifoAddress[channel], &dataStartAddress);

  // Getting the number of pages in the buffer, in case setting the addresses caused come changes.
  getNumOfPages(card, channel);

  /** Initializing the software FIFO */
  initSwFifo(card->swFifoAddress[channel]);

  /** Resetting the card,according to the RESET LEVEL parameter*/
  resetCard(card->barAddress[channel], RESET_LEVEL);

  /** Setting the card to be able to receive data */ 
  if(startDataReceiving(card->buffer[channel], card->barAddress[channel]) == -1)
    return -1;

  /** Initializing the firmware FIFO, pushing (128) pages */
  initFwFifo(card->buffer[channel], card->barAddress[channel], 128);

  if(DATA_GENERATOR){
    /** Initializing the data generator according to the LOOPBACK parameter*/
    armDataGenerator(card->barAddress[channel]);

    /** Starting the data generator */
    startDataGenerator(card->barAddress[channel]);
  }
  else{
    if(!NO_RDYRX){

      uint64_t timeout = pci_loop_per_usec*DDL_RESPONSE_TIME,
	       respCycle = pci_loop_per_usec*DDL_RESPONSE_TIME;

      // Clearing SIU/DIU status.

      if (rorcCheckLink(card->barAddress[channel]) != RORC_STATUS_OK){
        printf("SIU not seen. Can not clear SIU status.\n");
        return -1;
      }
      else
      {
        if(ddlReadSiu(card->barAddress[channel], 0, timeout) != -1){
	  printf("SIU status cleared.\n");
	}
      }
    
      if(ddlReadDiu(card->barAddress[channel], 0, timeout) != -1){
        printf("DIU status cleared.\n");
      }
      // RDYRX command to FEE
      ret = rorcStartTrigger(card->barAddress[channel], respCycle, &stw);
      if (ret == RORC_LINK_NOT_ON)
	{
	  printf("Error: LINK IS DOWN, RDYRX command can not be sent.\n");
	  return -1;
	}
      if (ret == RORC_STATUS_ERROR)
	{
	  printf("Error: RDYRX command can not be sent. I stop\n");
	  return -1;
	}
      if (ret == RORC_NOT_ACCEPTED)
	printf(" No reply arrived for RDYRX in timeout %lld usec\n", (unsigned long long)DDL_RESPONSE_TIME); 
      else
	printf(" FEE accepted the RDYDX command. Its reply: 0x%08lx\n",
	       stw.stw);
      //open_transaction = 1;//For EOBTR
    }
  }
  
  return 0;
}

int stopDMA(Card *card, int channel)
{
  uint64_t timeout = pci_loop_per_usec*DDL_RESPONSE_TIME;
  int ret;
  stword_t stw;

  // Stopping receiving data
  if(DATA_GENERATOR){
    rorcStopDataReceiver(card->barAddress[channel]);   
  }
  else if(!NO_RDYRX){
    // Sending EOBTR to FEE.
    ret = rorcStopTrigger(card->barAddress[channel], timeout, &stw);
    if (ret == RORC_LINK_NOT_ON){
      printf("Error: LINK IS DOWN, EOBTR command can not be sent. I stop\n");
      return -1;
    }
    if (ret == RORC_STATUS_ERROR){
      printf("Error: EOBTR command can not be sent. I stop\n");
      return -1;
    }
    printf(" EOBTR command sent to the FEE\n");
    //open_transaction = 0;
    if (ret != RORC_NOT_ACCEPTED){
      printf(" FEE accepted the EOBTR command. Its reply: 0x%08lx\n", stw.stw);
      return -1;
    }
  }

  return 0;
}

int checkPageWritten(Card *card, int channel, int *writeIndex, int *pushed)
{
  // If data has arrived:
  if(dataArrived(card->swFifoAddress[channel], *writeIndex)){
    // Setting software FIFO entry back to its initial state.
    setSwFifoEntry(card->swFifoAddress[channel], *writeIndex);

    // Indicating that the data needs to be read out.
    pushed[*writeIndex] = 0;

    // Incrementing the FIFO index.
    *writeIndex = (*writeIndex+1)%128;

    // Sleeping random amount of time (max SLEEP_TIME) to simulate loaded LDC.
    if(SLEEP_TIME){
      usleep((double)SLEEP_TIME*rand()/RAND_MAX);
    }

    return 0;
  }
  
  //if an error has occurred then:
  //if( error..) return -1;

  // If data has not arrived, return 1.
  return 1;
}

int checkPageRead(Card *card, int channel, int *readIndex, int *pushed, int *nextPage)
{
    // If data has arrived and has been read out:
  if(!dataArrived(card->swFifoAddress[0], *readIndex) && !pushed[*readIndex]){

    // Sleeping everytime a random amount of time (max LOAD_TIME) before a page is pushed.
    if(LOAD_TIME){
      usleep((double)LOAD_TIME*rand()/RAND_MAX);
    }

    // next page has to be between 0 and numberOfPages-1
    *nextPage = *nextPage%card->numberOfPages[channel];

    // Pushing new page.
    pushNewPage(card->barAddress[channel], pageToDAddress(*nextPage, card->buffer[channel]), *readIndex);
    // Indicating that the data has been read out.
    pushed[*readIndex] = 1;

    // Incrementing the FIFO index.
    *readIndex = (*readIndex+1)%128;

    // Jumping to the following page that needs to be pushed.
    (*nextPage)++;

    return 0;
  }
  
  // If data has not arrived or has not been read out:
  return 1;
}

int startDummyDMA(Card *card, int channel, int serial)
{
  char path[72];

  int i;
  *(card->map[channel]+(FULL_OFFSET-4-128*4)/sizeof(uint32_t)) = 1;
  for(i = 0; i < 128; i++){
    *(card->map[channel]+ (128*8+4)/sizeof(uint32_t)+i) = -1;
  }

  sprintf(path, "/root/pda/pda-11.0.7/test/dummyDataGenerator/dummyDataGenerator -c%d -s%d &", channel, serial);

  system(path);
  printf("%s\n", path);
  return 0;
}

int stopDummyDMA(Card *card, int channel)
{

  *(card->map[channel]+(FULL_OFFSET-4-128*4)/sizeof(uint32_t)) = 0;

  // Giving time for the dummy data generator to close.
  printf("\nWait 5 seconds until dummyDataGenerator closes ...\n");
  sleep(5);

  return 0;
}
