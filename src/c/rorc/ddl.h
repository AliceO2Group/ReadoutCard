#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "stdint.h"
#include "linux/types.h"

#ifdef __cplusplus
extern "C" {
#endif

stword_t ddlReadStatus(volatile void *buff);
long long int ddlWaitStatus(volatile void *buff, long long int timeout);
int ddlSendCommand(volatile void *buff, int dest, __u32 command, int transid, __u32 param, long long int time);
stword_t ddlReadCTSTW(volatile void *buff, int transid, int destination, long long int time, int pci_loop_per_usec);
long long int ddlWaitStatus(volatile void *buff, long long int timeout);
stword_t ddlReadStatus(volatile void *buff);
long ddlReadDiu(volatile void *buff, int transid, long long int time, int pci_loop_per_usec);
long ddlReadSiu(volatile void *buff, int transid, long long int time, int pci_loop_per_usec);
void ddlInterpretIFSTW(__u32 ifstw, char* pref, char* suff, int diu_version);
void ddlInterpret_OLD_IFSTW(__u32 ifstw, char* pref, char* suff);
void ddlInterpret_NEW_IFSTW(__u32 ifstw, char *pref, char *suff);
unsigned long ddlResetSiu(volatile void *buff, int print, int cycle, long long int time, int diu_version,
    int pci_loop_per_usec);
unsigned long ddlLinkUp(volatile void *buff, int master, int print, int stop, long long int time, int diu_version,
    int pci_loop_per_usec);
unsigned long ddlLinkUp_OLD(volatile void *buff, int master, int print, int stop, long long int time, int diu_version,
    int pci_loop_per_usec);
unsigned long ddlLinkUp_NEW(volatile void *buff, int master, int print, int stop, long long int time, int diu_version,
    int pci_loop_per_usec);
int ddlFindDiuVersion(volatile void *buff, int pci_loop_per_usec, int *rorc_revision, int *diu_version);
int ddlSetSiuLoopBack(volatile void *buff, long long int timeout, int pci_loop_per_usec, stword_t *stw);

#ifdef __cplusplus
} /* extern "C" */
#endif

