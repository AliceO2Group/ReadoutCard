#include "rorc.h"

char* receivedOrderedSet[] = {"SRST", "Not_Op", "Oper", "L_Init",
			      "Idle", "Xoff", "Xon", "data or delimiter", "unknown ordered set"};

char* remoteStatus[] = {"Power On Reset", "Offline", "Online", "Waiting for PO",
			"Offline No Signal", "Offline LOS", "No Optical Signal", "undefined"};

void elapsed(struct timeval *tv2, struct timeval *tv1, 
             int *dsec, int *dusec){
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  *dsec = tv2->tv_sec - tv1->tv_sec;
  *dusec = tv2->tv_usec - tv1->tv_usec;
  if (*dusec < 0){
    (*dsec)--;
    *dusec += MEGA;
  }
}

void setLoopPerSec(long long int *loop_per_usec, double *pci_loop_per_usec, volatile void *buff){
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  struct timeval tv1, tv2;
  int dsec, dusec;
  double dtime, max_loop;
  int i;

  max_loop = 1000000;
  gettimeofday(&tv1, NULL);

  for (i = 0; i < max_loop; i++){};

  gettimeofday(&tv2, NULL);
  elapsed(&tv2, &tv1, &dsec, &dusec);
  dtime = (double)dsec * MEGA + (double)dusec;
  *loop_per_usec = (double)max_loop/dtime;
  if (*loop_per_usec < 1)
    *loop_per_usec = 1;
  // printf("memory loop_per_usec: %lld\n", prorc_dev->loop_per_usec);
  
  /* calibrate PCI loop time for time-outs */
  max_loop = 1000;
  gettimeofday(&tv1, NULL);

  for (i = 0; i < max_loop; i++)
    rorcCheckRxStatus(buff);
  
  gettimeofday(&tv2, NULL);
  elapsed(&tv2, &tv1, &dsec, &dusec);
  dtime = (double)dsec * MEGA + (double)dusec;
  *pci_loop_per_usec = (double)max_loop/dtime;
}

int roundPowerOf2(int number){
  int logNum;

  logNum = logi2(number);
  return (1 << logNum);
}

int logi2(unsigned int number){
  int i;
  unsigned int mask = 0x80000000;

  for (i = 0; i < 32; i++){
    if (number & mask)
      break;
    mask = mask >> 1;
  }

  return (31 - i);  
}
