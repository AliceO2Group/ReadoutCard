/// \file interface.c
/// \brief Example for DMA using the API for the PDA and the CRORC/CRU cards.
///
///  Last updated: 24/05/2016
///
/// \author Tuan Mate Nguyen <tuan.mate.nguyen@cern.ch>

#define _XOPEN_SOURCE // For sigaction to work
#include "header.h"
#include <signal.h>

int writeIndex, readIndex;     // Pointers to the software FIFO entries to which those pages
                            // are assigned where the next data block will be written and
                            // will be read out, respectively.
int count,         // Total number of written pages.
    nextPage,     // Index of the next page to be pushed.
    pushedPages,  // Total number of pushed pages.
    numberOfPages;  // Number of pages in the buffer.

int run;           // Indicator for DMA start (run = 1)  and end (run = 0).

/// Interrupt handler to stop DMA.
/// \param signum  The signal number to which the handling function is set (SIGINT int this case)
void terminationHandler(int signum)
{
  // This will end the while loop (while(run))
  run = 0;

  // Re-establishing the signal handler function.
  //signal(SIGINT, terminationHandler);
}

int main(int argc, char** argv)
{
  Card* card;               // Struct for the accessed card.
  int pushed[128] = {0};   // Array to keep track of read pages (0: wasn't read out, 1:was read out).

  int checkReturnValue;    // Auxiliary variable for checking return value.

  struct timeval start, end; // Structs needed for time interval measurement.
  int sec, usec;             // Seconds and microseconds passed.
  double diff;               // Total time passed.

  // Creating Card object.
  card = createCard();

  // Open channel 0 on the card with serial number 33333.
  if(openCard(card, 0, 33333) == -1){
    perror("Couldn't open card");
    exit(EXIT_FAILURE);
  }

  // Allocate DMA buffer (buffer size is given in the config file).
  if(allocateMemory(card, 0) == -1){
    perror("Couldn't allocate memory");
    closeCard(card);
    exit(EXIT_FAILURE);
  }
  
  // Initializing the readout array.
  for(int i = 0; i < 128; i++){
    pushed[i] = 1;
  }

  // Setting up the indices.
  nextPage = 128;
  writeIndex = 0;
  readIndex = 0;
  count = 0;
  run = 1;

  // Setting up the interrupt handler.
  struct sigaction act;
  act.sa_handler = terminationHandler;
  act.sa_flags = 0;
  sigaction(SIGINT, &act, NULL);

  // Starting the time measurement.
  gettimeofday(&start, NULL);

  // Starting DMA.
  printf("%s\n", argv[0]);
  startDMA(card, 0);

  // DMA loop.
  while(run){
    checkReturnValue = checkPageWritten(card, 0, &writeIndex, pushed);
    //printf("%d\n");
    if(checkReturnValue == 0){
      count++;
      //incomingDataSize += *(card->swFifoAddress[0]+((writeIndex==0)?127:writeIndex-1)*2)*4;
      //printf("%d\n",  *(card->swFifoAddress[0]+((writeIndex==0)?127:writeIndex-1)*2));
    }
    else if(checkReturnValue == -1){
      printf("\nAn error has occurred\n\n");
      break;
    }
    checkPageRead(card, 0, &readIndex, pushed, &nextPage);
  }//while(1)

  // Ending time measurement.
  gettimeofday(&end, NULL);

  // Ending DMA.
  stopDMA(card, 0);

  // Calculating and printing throughput.
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  if(usec<0){
    usec += 1000000;
    sec--;
  }
  diff = (double)sec+(double)usec/1000000;
  printf("\n\nThroughput: %f Gb/s\n\n", (double)count/diff/1024/1024*DATA_LENGTH/1024);
  //printf("count = %d\n", count);

  // Uncomment section for dummyDMA example.  
  /*  // Setting back the indices for the next (dummy) DMA.
  writeIndex = 0;
  run = 1;
  count = 0;
  nextPage = 0;

  // Starting the time measurement.
  gettimeofday(&start, NULL);
  
  // Starting dummy DMA. 
  startDummyDMA(card, 0, 33333);
  
  // Dummy DMA loop.
  while(run){
    if( *(card>map[0]+ (128*8+4)/sizeof(uint32_t)+writeIndex) == -1){
      *(card->map[0]+ (128*8+4)/sizeof(uint32_t)+writeIndex) = nextPage;
      nextPage = (nextPage+1)%card->.numberOfPages[0];
      writeIndex = (writeIndex+1)%128;
      count++;
    }
  }
  // Ending time measurement.
  gettimeofday(&end, NULL);
  
  // Ending dummy DMA.
  stopDummyDMA(card, 0);  
 
  // Calculating and printing throughput once again.
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  if(usec<0){
    usec += 1000000;
    sec--;
  }
  diff = (double)sec+(double)usec/1000000;
  printf("\nThroughput: %f Gb/s\n\n", (double)count/diff/1024/1024*1024/1024);
  printf("count = %d\n", count);
  */

  // Closing card.
  closeChannel(card, 0);
  closeCard(card);

  // TEST for allocating memory buffer from the userspace.
  /*
  uint32_t* buffer; // Userspace allocated memory buffer.
  uint64_t* ids;  // Array for found buffer IDs.
  int numOfBuffers; // Number of found buffers.

  openCard(card, 0,33333);
  // Getting and printing the number of buffers before assigning any, this should give 0.
  numOfBuffers = PciDevice_getListOfBuffers(card->device, &ids);
  printf("Number of buffers: %d\n", numOfBuffers);
  for(int i = 0; i < numOfBuffers; i++){
    printf("id%d: %d\n", i, ids[i]);
  }

  // Allocating page aligned memory buffer.
  posix_memalign((void**)&buffer, pathconf("/",_PC_REC_XFER_ALIGN), 32*1024*1024);

  // Assigning the allocated buffer to the device.
  PciDevice_registerDMABuffer(card->device, 444, (void*)buffer, 32*1024*1024, card->buffer[0]);

  // Getting and printing the number of buffers before assigning any, we should now see
  // the newly allocated buffer.
  numOfBuffers = PciDevice_getListOfBuffers(card->device, &ids);
  printf("Number of buffers: %d\n", numOfBuffers);
  for(int i = 0; i < numOfBuffers; i++){
    printf("id%d: %d\n", i, ids[i]);
  }
  closeCard(card);
  */
 
  // TEST for opening several channel on the same card.
  /*
  openCard(card, 0, 33333);
  if(allocateMemory(card, 0) == -1){
    perror("Couldn't allocate memory");
    closeCard(card);
    exit(EXIT_FAILURE);
  }

  printf("Open channel 1\n");
  openCard(card, 1, 33333);

  printf("Open channel 2\n");
  openCard(card, 2, 0);

  if(allocateMemory(card, 1) == -1){
    perror("Couldn't allocate memory");
    closeCard(card);
    exit(EXIT_FAILURE);
  }
  printf("Open channel 3\n");
  openCard(card, 3, 33333);
  if(allocateMemory(card, 3) == -1){
    perror("Couldn't allocate memory");
    closeCard(card);
    exit(EXIT_FAILURE);
  }
  printf("Open channel 4\n");
  openCard(card, 4, 33333);
  if(allocateMemory(card, 4) == -1){
    perror("Couldn't allocate memory");
    closeCard(card);
    exit(EXIT_FAILURE);
  }
  printf("Open channel 5\n");
  openCard(card, 5, 33333);
  printf("Alloc 5\n");
  if(allocateMemory(card, 5) == -1){
    perror("Couldn't allocate memory");
    closeCard(card);
    exit(EXIT_FAILURE);
  }
*/
 
  // TEST for deleting same buffer multiple times.
  /* 
  PciDevice* dev;
  DMABuffer* buf;
  DeviceOperator_getPciDevice(dop, &dev, 0);
  PciDevice_getDMABuffer(dev, 5, &buf);
  PciDevice_delete(dev, PDA_DELETE);

  uint64_t* ids;  // Array for found buffer IDs.
  int numOfBuffers; // Number of found buffers.
  numOfBuffers = PciDevice_getListOfBuffers(card->device, &ids);
  printf("Number of buffers: %d\n", numOfBuffers);
  for(int x = 0; x < numOfBuffers; ++x){
    printf("ID%d = %d\n", x, ids[x]);
  }

  closeCard(card);
*/

  // Free Card object.
  deleteCard(card);

  printf("Card is closed, quitting...\n\n");
  
  return 0;
}
