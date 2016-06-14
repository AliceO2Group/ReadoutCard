/// \file interface.c
/// \brief Example for DMA using the API for the PDA and the CRORC/CRU cards.
///
///  Last updated: 26/04/2016
///
/// \author Tuan Mate Nguyen <tuan.mate.nguyen@cern.ch>

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
  run = 0;
  //printf("\n%d %d\n", writeIndex, readIndex);
  //printf("%d, %d\n", count, pushedPages);
  //printf("%d %d\n", dataArrived(sw_fifo, writeIndex), dataArrived(sw_fifo, readIndex));
  //exit(EXIT_FAILURE);
}

int main()
{
  Card card;               // Struct for the accessed card.
  int pushed[128] = {0};   // Array to keep track of read pages (0: wasn't read out, 1:was read out).

  int checkReturnValue;    // Auxiliary variable for checking return value.

  struct timeval start, end; // Structs needed for time interval measurement.
  int sec, usec;             // Seconds and microseconds passed.
  double diff;               // Total time passed.

  // Finding cards, get their types and initializing accordingly.
  findCards();
  
  // Open channel 0 on the card.
  openCard(&card, 0, 0);
  
  // Allocate DMA buffer (buffer size is given in the config file).
  allocateMemory(&card, 0);

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
  signal(SIGINT, terminationHandler);

  // Starting the time measurement.
  gettimeofday(&start, NULL);

  // Starting DMA.
  startDMA(&card, 0);

  // DMA loop.
  while(run){
    checkReturnValue = checkPageWritten(&card, 0, &writeIndex, pushed);
    if(checkReturnValue == 0){
      count++;
      //incomingDataSize += *(card.swFifoAddress[0]+((writeIndex==0)?127:writeIndex-1)*2)*4;
      //printf("%d\n",  *(card.swFifoAddress[0]+((writeIndex==0)?127:writeIndex-1)*2));
    }
    else if(checkReturnValue == -1){
      printf("\nAn error has occurred\n\n");
      break;
    }
    checkPageRead(&card, 0, &readIndex, pushed, &nextPage);
  }//while(1)

  // Ending time measurement.
  gettimeofday(&end, NULL);

  // Ending DMA.
  stopDMA(&card, 0);

  // Calculating and printing throughput.
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  if(usec<0){
    usec += 1000000;
    sec--;
  }
  diff = (double)sec+(double)usec/1000000;
  printf("\n\nThroughput: %f Gb/s\n\n", (double)count/diff/1024/1024*DATA_LENGTH/1024);
  printf("%d\n", count);

  // Setting back the indices for the next (dummy) DMA.
  writeIndex = 0;
  run = 1;
  count = 0;
  nextPage = 0;

  // Starting the time measurement.
  gettimeofday(&start, NULL);

  // Starting dummy DMA. 
  startDummyDMA(&card, 0, 0);

  // Dummy DMA loop.
  while(run){
    if( *(card.map[0]+ (128*8+4)/sizeof(uint32_t)+writeIndex) == -1){
      *(card.map[0]+ (128*8+4)/sizeof(uint32_t)+writeIndex) = nextPage;
      nextPage = (nextPage+1)%card.numberOfPages[0];
      writeIndex = (writeIndex+1)%128;
      count++;
    }
  }
  // Ending time measurement.
  gettimeofday(&end, NULL);

  // Ending dummy DMA.
  stopDummyDMA(&card, 0);
  
  // Calculating and printing throughput once again.
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  if(usec<0){
    usec += 1000000;
    sec--;
  }
  diff = (double)sec+(double)usec/1000000;
  printf("\n\nThroughput: %f Gb/s\n\n", (double)count/diff/1024/1024*1024/1024);
  printf("%d\n", count);

  // Closing card.
  closeCard(&card);

  return 0;
}
