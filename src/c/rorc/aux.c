#include "aux.h"
#include "rorc.h"
#include <pda.h>
#include "rorc_macros.h"
#include <unistd.h>

//char* receivedOrderedSet[] = {"SRST", "Not_Op", "Oper", "L_Init",
//			      "Idle", "Xoff", "Xon", "data or delimiter", "unknown ordered set"};
//char* remoteStatus[] = {"Power On Reset", "Offline", "Online", "Waiting for PO",
//			"Offline No Signal", "Offline LOS", "No Optical Signal", "undefined"};

void elapsed(struct timeval *tv2, struct timeval *tv1,
             int *dsec, int *dusec);
int roundPowerOf2(int number);
int logi2(unsigned int number);
int trim(char *string);

void elapsed(struct timeval *tv2, struct timeval *tv1,
             int *dsec, int *dusec){
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  *dsec = tv2->tv_sec - tv1->tv_sec;
  *dusec = tv2->tv_usec - tv1->tv_usec;
  if (*dusec < 0){
    (*dsec)--;
    *dusec += 1000000;
  }
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

int trim(char *string)
{
  // cut trailing white spaces and end-of-lines
  int i;
  char ch;

  i = strlen(string);
  while(i)
  {
    ch = string[i-1];
    if ( (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == 0x0d) )
      string[--i] = '\0';
    else
      break;
  }

  return(i);
}
