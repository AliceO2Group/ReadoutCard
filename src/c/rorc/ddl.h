#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "stdint.h"
#include "linux/types.h"

#ifdef __cplusplus
extern "C" {
#endif

stword_t ddlReadStatus(uintptr_t bar);
long long int ddlWaitStatus(uintptr_t bar, long long int timeout);
int ddlSendCommand(uintptr_t bar, int dest, __u32 command, int transid, __u32 param, long long int time);
stword_t ddlReadCTSTW(uintptr_t bar, int transid, int destination, long long int time, int pci_loop_per_usec);
long long int ddlWaitStatus(uintptr_t bar, long long int timeout);
stword_t ddlReadStatus(uintptr_t bar);
long ddlReadDiu(uintptr_t bar, int transid, long long int time, int pci_loop_per_usec);
long ddlReadSiu(uintptr_t bar, int transid, long long int time, int pci_loop_per_usec);
void ddlInterpretIFSTW(__u32 ifstw, char* pref, char* suff, int diu_version);
void ddlInterpret_OLD_IFSTW(__u32 ifstw, char* pref, char* suff);
void ddlInterpret_NEW_IFSTW(__u32 ifstw, char *pref, char *suff);
unsigned long ddlResetSiu(uintptr_t bar, int print, int cycle, long long int time, int diu_version,
    int pci_loop_per_usec);
unsigned long ddlLinkUp(uintptr_t bar, int master, int print, int stop, long long int time, int diu_version,
    int pci_loop_per_usec);
unsigned long ddlLinkUp_OLD(uintptr_t bar, int master, int print, int stop, long long int time, int diu_version,
    int pci_loop_per_usec);
unsigned long ddlLinkUp_NEW(uintptr_t bar, int master, int print, int stop, long long int time, int diu_version,
    int pci_loop_per_usec);
int ddlFindDiuVersion(uintptr_t bar, int pci_loop_per_usec, int *rorc_revision, int *diu_version);
int ddlSetSiuLoopBack(uintptr_t bar, long long int timeout, int pci_loop_per_usec, stword_t *stw);

#ifdef __cplusplus
} /* extern "C" */
#endif

