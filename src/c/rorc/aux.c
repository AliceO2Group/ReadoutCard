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

unsigned initFlash(uint32_t *buff, unsigned address, int sleept)
{
  unsigned stat;

  // Clear Status register
  rorcWriteReg(buff, F_IFDSR, 0x03000050);
  usleep(10*sleept);

  // Set ASYNCH mode (Configuration Register 0xBDDF)
  rorcWriteReg(buff, F_IFDSR, 0x0100bddf);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x03000060);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x03000003);
  usleep(sleept);

  // Read Status register  
  rorcWriteReg(buff, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x03000070);
  usleep(sleept);
  stat = readFlashStatus(buff, 1);

  return (stat);
}

unsigned readFlashStatus(uint32_t *buff, int sleept)
{
  unsigned stat;

  rorcWriteReg(buff, F_IFDSR, 0x04000000);
  usleep(sleept);
  stat = rorcReadReg(buff, F_IADR);

  return (stat);
}

int checkFlashStatus(uint32_t *buff, int timeout)
{
  unsigned stat;
  int i = 0;

  while (1) 
  {
    stat = readFlashStatus(buff, 1);
    if (stat == 0x80)
      break;
    if (timeout && (++i >= timeout))
      return (RORC_TIMEOUT);
    usleep(100);
  }

  return (RORC_STATUS_OK);

}

int unlockFlashBlock(uint32_t *buff, unsigned address, int sleept)
{
  int ret;
 
  rorcWriteReg(buff, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x03000060);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x030000d0);
  usleep(sleept);
  ret = checkFlashStatus(buff, MAX_WAIT);

  return (ret);
}

int eraseFlashBlock(uint32_t *buff, unsigned address, int sleept)
{
  int ret;
 
  rorcWriteReg(buff, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x03000020);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x030000d0);
  usleep(sleept);
  ret = checkFlashStatus(buff, MAX_WAIT);

  return (ret);
}

int writeFlashWord(uint32_t *buff, unsigned address, int value, int sleept)
{
  int ret;

  rorcWriteReg(buff, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x03000040);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, value);
  usleep(sleept);
  ret = checkFlashStatus(buff, MAX_WAIT);

  return (ret);
}

int readFlashWord(uint32_t *buff, unsigned address, __u8 *data, int sleept)
{
  unsigned stat;

  rorcWriteReg(buff, F_IFDSR, address);
  usleep(sleept);
  rorcWriteReg(buff, F_IFDSR, 0x030000ff);
  usleep(sleept);
  stat = readFlashStatus(buff, sleept);
  *data = (stat & 0xFF00) >> 8;
  *(data+1) = stat & 0xFF;
  return (RORC_STATUS_OK);
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
